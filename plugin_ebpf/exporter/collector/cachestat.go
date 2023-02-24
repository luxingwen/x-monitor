/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:43:48
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 16:49:58
 */

package collector

import (
	"time"

	"github.com/cilium/ebpf/link"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sourcegraph/conc"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

func init() {
	registerEBPFModule(cacheStateModuleName, newCacheStatModule)
}

type cacheStatModule struct {
	name     string
	stopChan chan struct{}
	wg       conc.WaitGroup
	// prometheus对象
	// eBPF对象
	objs  *bpfmodule.XMCacheStatObjects
	links []link.Link
}

func newCacheStatModule(name string) (eBPFModule, error) {
	csm := new(cacheStatModule)
	csm.name = name
	csm.stopChan = make(chan struct{})

	spec, err := bpfmodule.LoadXMCacheStat()
	if err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' LoadXMCacheStat failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	// set spec
	// output: cachestat.bpf.o map name:'xm_page_cache_ops_count', spec:'Hash(keySize=8, valueSize=8, maxEntries=4, flags=0)'
	for name, mapSpec := range spec.Maps {
		glog.Infof("cachestat.bpf.o map name:'%s', info:'%s', key.TypeName:'%s', value.TypeName:'%s'",
			name, mapSpec.String(), mapSpec.Key.TypeName(), mapSpec.Value.TypeName())
	}

	// cachestat.bpf.o prog name:'xm_kp_cs_atpcl', type:'Kprobe', attachType:'None', attachTo:'add_to_page_cache_lru', sectionName:'kprobe/add_to_page_cache_lru'
	for name, progSpec := range spec.Programs {
		glog.Infof("cachestat.bpf.o prog name:'%s', type:'%s', attachType:'%s', attachTo:'%s', sectionName:'%s'",
			name, progSpec.Type.String(), progSpec.AttachType.String(), progSpec.AttachTo, progSpec.SectionName)
	}

	// use spec init cacheStat object
	csm.objs = new(bpfmodule.XMCacheStatObjects)
	if err := spec.LoadAndAssign(csm.objs, nil); err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' LoadAndAssign failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	if links, err := AttachObjPrograms(csm.objs.XMCacheStatPrograms, spec.Programs); err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' AttachObjPrograms failed.", name)
		glog.Error(err.Error())
		return nil, err
	} else {
		csm.links = links
	}

	cacheStatEventProcessor := func() {
		glog.Infof("eBPFModule:'%s' start gathering eBPF data...", csm.name)

	loop:
		for {
			select {
			case <-csm.stopChan:
				glog.Infof("eBPFModule:'%s' receive stop notify", csm.name)
				break loop
			default:
				time.Sleep(time.Millisecond * 50)
			}
		}
	}

	csm.wg.Go(cacheStatEventProcessor)

	return csm, nil
}

func (csm *cacheStatModule) Update(ch chan<- prometheus.Metric) error {
	return nil
}

// Stop stops the eBPF module. This function is called when the eBPF module is removed from the system.
func (csm *cacheStatModule) Stop() {
	glog.Infof("Stop eBPFModule:'%s'", csm.name)

	close(csm.stopChan)
	if panic := csm.wg.WaitAndRecover(); panic != nil {
		glog.Errorf("eBPFModule:'%s' panic: %v", csm.name, panic.Error())
	}

	if csm.links != nil {
		for _, link := range csm.links {
			link.Close()
		}
		csm.links = nil
	}

	if csm.objs != nil {
		csm.objs.Close()
		csm.objs = nil
	}
}
