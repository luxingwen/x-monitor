/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:44:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-02 16:46:18
 */

package collector

import (
	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

type runqLatModule struct {
	eBPFBaseModule
	// prometheus对象
	// eBPF对象
	objs *bpfmodule.XMRunQLatObjects
}

func init() {
	registerEBPFModule(runqLatencyModuleName, newRunQLatencyModule)
}

func newRunQLatencyModule(name string) (eBPFModule, error) {
	rqlModule := &runqLatModule{
		eBPFBaseModule: eBPFBaseModule{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
	}

	var err error
	rqlModule.objs = new(bpfmodule.XMRunQLatObjects)
	rqlModule.links, err = runEBPFModule(name, rqlModule.objs, bpfmodule.LoadXMRunQLat)
	if err != nil {
		rqlModule.objs.Close()
		rqlModule.objs = nil
		return nil, err
	}

	return rqlModule, nil
}

func (rql *runqLatModule) Update(ch chan<- prometheus.Metric) error {
	return nil
}

// Stop closes the runqLatModule and stops the eBPF module.
func (rql *runqLatModule) Stop() {
	rql.stop()

	if rql.objs != nil {
		rql.objs.Close()
		rql.objs = nil
	}

	glog.Infof("eBPFModule:'%s' stopped.", rql.name)
}
