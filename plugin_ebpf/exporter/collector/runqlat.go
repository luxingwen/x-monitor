/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:44:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-02 16:46:18
 */

package collector

import (
	"errors"

	"github.com/cilium/ebpf"
	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

type runqLatModule struct {
	*eBPFBaseModule
	// prometheus对象
	// eBPF对象
	objs *bpfmodule.XMRunQLatObjects
}

const _runqLatHistMapKey int32 = -1

func init() {
	registerEBPFModule(runqLatencyModuleName, newRunQLatencyModule)
}

func newRunQLatencyModule(name string) (eBPFModule, error) {
	rqlM := &runqLatModule{
		eBPFBaseModule: &eBPFBaseModule{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
	}

	var err error
	rqlM.objs = new(bpfmodule.XMRunQLatObjects)
	rqlM.links, err = runEBPF(name, rqlM.objs, bpfmodule.LoadXMRunQLat)
	if err != nil {
		rqlM.objs.Close()
		rqlM.objs = nil
		return nil, err
	}

	interval := config.EBPFModuleInterval(name)

	eBPFTracing := func() {
		glog.Infof("eBPFModule:'%s' start gathering eBPF data...", rqlM.name)
		rqlM.gatherTimer.Reset(interval)

		var hist bpfmodule.XMRunQLatXmRunqlatHist
		// var key uint32 = -1

	loop:
		for {
			select {
			case <-rqlM.stopChan:
				glog.Infof("eBPFModule:'%s' receive stop notify", rqlM.name)
				break loop
			case <-rqlM.gatherTimer.Chan():
				// gather eBPF data
				if err := rqlM.objs.XmRunqlatHistsMap.Lookup(_runqLatHistMapKey, &hist); err != nil {
					if !errors.Is(err, ebpf.ErrKeyNotExist) {
						glog.Errorf("eBPFModule:'%s' Lookup error. err:%s", rqlM.name, err.Error())
					}
				} else {
					glog.Infof("eBPFModule:'%s' hist:%#+v", rqlM.name, hist)

					if err := rqlM.objs.XmRunqlatHistsMap.Delete(_runqLatHistMapKey); err != nil {
						glog.Errorf("eBPFModule:'%s' Delete error. err:%s", rqlM.name, err.Error())
					}
				}

				rqlM.gatherTimer.Reset(interval)
			}
		}
	}

	rqlM.wg.Go(eBPFTracing)

	return rqlM, nil
}

func (rqlM *runqLatModule) Update(ch chan<- prometheus.Metric) error {
	return nil
}

// Stop closes the runqLatModule and stops the eBPF module.
func (rqlM *runqLatModule) Stop() {
	rqlM.stop()

	if rqlM.objs != nil {
		rqlM.objs.Close()
		rqlM.objs = nil
	}

	glog.Infof("eBPFModule:'%s' stopped.", rqlM.name)
}
