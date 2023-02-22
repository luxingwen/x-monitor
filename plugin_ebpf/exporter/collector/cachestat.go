/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:43:48
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 16:49:58
 */

package collector

import (
	"sync"
	"time"

	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

func init() {
	registerEBPFModule(cacheStateModuleName, newCacheStatModule)
}

type cacheStatModule struct {
	name     string
	stopChan chan struct{}
	wg       sync.WaitGroup
	// prometheus对象
	// eBPF对象
	objs *bpfmodule.XMCacheStatObjects
}

func newCacheStatModule(name string) (eBPFModule, error) {
	module := new(cacheStatModule)
	module.name = name
	module.stopChan = make(chan struct{})

	spec, err := bpfmodule.LoadXMCacheStat()
	if err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' LoadXMCacheStat failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	// set spec
	for name, mapSpec := range spec.Maps {
		glog.Infof("cachestat.bpf.o map name:'%s', spec:'%s'", name, mapSpec.String())
	}

	// use spec init cacheStat object
	module.objs = new(bpfmodule.XMCacheStatObjects)
	if err := spec.LoadAndAssign(module.objs, nil); err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' LoadAndAssign failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	cacheStatProducer := func() {
		glog.Infof("eBPFModule:'%s' start gathering eBPF data...", module.name)

		defer func() {
			glog.Warningf("eBPFModule:'%s' will exit", module.name)
			module.wg.Done()

			if err := recover(); err != nil {
				panicStack := calmutils.CallStack(3)
				glog.Errorf("Panic. err:%v, stack:%s", err, panicStack)
			}
		}()

	loop:
		for {
			select {
			case <-module.stopChan:
				glog.Infof("eBPFModule:'%s' receive stop notify", module.name)
				break loop
			default:
				time.Sleep(time.Millisecond * 50)
			}
		}
	}

	module.wg.Add(1)
	go cacheStatProducer()

	return module, nil
}

func (cs *cacheStatModule) Update(ch chan<- prometheus.Metric) error {
	return nil
}

// Stop stops the eBPF module. This function is called when the eBPF module is removed from the system.
func (cs *cacheStatModule) Stop() {
	glog.Infof("Stop eBPFModule:'%s'", cs.name)

	close(cs.stopChan)
	cs.wg.Wait()

	if cs.objs != nil {
		cs.objs.Close()
		cs.objs = nil
	}
}
