/*
 * @Author: CALM.WU
 * @Date: 2023-02-17 14:33:02
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-17 14:37:55
 */

package collector

import (
	"reflect"
	"strings"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/sourcegraph/conc"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

const (
	cacheStateModuleName  = "cachestat"
	runqLatencyModuleName = "runqlat"
)

type eBPFBaseModule struct {
	// module
	name        string
	stopChan    chan struct{}
	wg          conc.WaitGroup
	gatherTimer *calmutils.Timer
	// ebpf
	links []link.Link
}

type __load func() (*ebpf.CollectionSpec, error)
type __assign func(interface{}, *ebpf.CollectionOptions) error

func runEBPF(name string, objs interface{}, loadF __load) ([]link.Link, error) {
	spec, err := loadF()
	if err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' load spec failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	if err := spec.LoadAndAssign(objs, nil); err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' assign objs failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	// 得到objs的xxxPrograms对象
	objsV := reflect.Indirect(reflect.ValueOf(objs))
	objsT := objsV.Type()
	objsNumFields := objsT.NumField()

	var links []link.Link

	for i := 0; i < objsNumFields; i++ {
		objsVField := objsT.Field(i)
		if objsVField.Type.Kind() == reflect.Struct &&
			strings.Contains(objsVField.Name, "Programs") {
			links, err = AttachObjPrograms(objsVField, spec.Programs)
			if err != nil {
				err = errors.Wrapf(err, "eBPFModule:'%s' AttachObjPrograms failed.", name)
				glog.Error(err.Error())
				return nil, err
			}
		}
	}
	glog.Infof("eBPFModule:'%s' start run successfully.", name)
	return links, nil
}

func (ebm *eBPFBaseModule) stop() {
	close(ebm.stopChan)
	if panic := ebm.wg.WaitAndRecover(); panic != nil {
		glog.Errorf("eBPFModule:'%s' panic: %v", ebm.name, panic.Error())
	}

	if ebm.links != nil {
		for _, link := range ebm.links {
			link.Close()
		}
		ebm.links = nil
	}

	if ebm.gatherTimer != nil {
		ebm.gatherTimer.Stop()
		ebm.gatherTimer = nil
	}
}
