/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:43:48
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-17 14:19:57
 */

package collector

import (
	"sync"
	"time"

	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

const (
	__moduleName = "cachestat"
)

func init() {
	registerEBPFModule(__moduleName, newCacheStatCollector)
}

type cacheStatCollector struct {
	name     string
	stopChan chan struct{}
	wg       sync.WaitGroup
	// prometheus对象
	// eBPF对象
}

func newCacheStatCollector() (eBPFModule, error) {
	module := new(cacheStatCollector)
	module.name = "cachestat"
	module.stopChan = make(chan struct{})

	eBPFKernelDataGather := func() {
		glog.Infof("eBPFModule:'%s' start gathering eBPF data...", __moduleName)

		defer func() {
			glog.Warningf("eBPFModule:'%s' will exit", __moduleName)
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
				glog.Infof("eBPFModule:'%s' receive stop notify", __moduleName)
				break loop
			default:
				time.Sleep(time.Millisecond * 50)
			}
		}
	}

	module.wg.Add(1)
	go eBPFKernelDataGather()

	return module, nil
}

func (cs *cacheStatCollector) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (cs *cacheStatCollector) Stop() {
	glog.Infof("Stop eBPFModule:'%s'", cs.name)

	close(cs.stopChan)
	cs.wg.Wait()

	// 释放eBPF对象
	// 释放prometheus对象
}
