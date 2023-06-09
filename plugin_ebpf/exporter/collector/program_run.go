/*
 * @Author: CALM.WU
 * @Date: 2023-02-17 14:33:02
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-27 11:21:10
 */

package collector

import (
	"reflect"
	"strings"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/golang/glog"
	"github.com/pkg/errors"
)

type __load func() (*ebpf.CollectionSpec, error)
type __rewriteConstVars func(*ebpf.CollectionSpec) error

func attatchToRun(name string, objs interface{}, loadF __load, rewriteConstVarsF __rewriteConstVars) ([]link.Link, error) {
	spec, err := loadF()
	if err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' load spec failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	if rewriteConstVarsF != nil {
		if err := rewriteConstVarsF(spec); err != nil {
			err = errors.Wrapf(err, "eBPFProgram:'%s' rewrite const variables failed.", name)
			glog.Error(err.Error())
			return nil, err
		}
	}

	if err := spec.LoadAndAssign(objs, nil); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' assign objs failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	// 得到objs的xxxPrograms对象
	objsV := reflect.Indirect(reflect.ValueOf(objs))
	objsT := objsV.Type()
	objsNumFields := objsV.NumField() //objsT.NumField()

	var links []link.Link

	for i := 0; i < objsNumFields; i++ {
		field := objsT.Field(i)
		if field.Type.Kind() == reflect.Struct && strings.Contains(field.Name, "Programs") {
			links, err = AttachObjPrograms(objsV.Field(i).Interface(), spec.Programs)
			if err != nil {
				err = errors.Wrapf(err, "eBPFProgram:'%s' AttachObjPrograms failed.", name)
				glog.Error(err.Error())
				return nil, err
			}
		}
	}
	glog.Infof("eBPFProgram:'%s' start attatchToRun successfully.", name)
	return links, nil
}

func (ebm *eBPFBaseProgram) stop() {
	close(ebm.stopChan)
	if recover := ebm.wg.WaitAndRecover(); recover != nil {
		glog.Errorf("eBPFProgram:'%s' recover: %v", ebm.name, recover.String())
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
