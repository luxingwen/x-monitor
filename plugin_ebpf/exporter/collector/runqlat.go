/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:44:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-02 16:46:18
 */

package collector

import (
	"errors"
	"sync"

	"github.com/cilium/ebpf"
	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

type runqLatModule struct {
	*eBPFBaseModule
	// prometheus对象，单位微秒
	runqLatHistogramDesc *prometheus.Desc
	sampleCount          uint64
	sampleSum            float64
	buckets              map[float64]uint64
	mu                   sync.Mutex
	// eBPF对象
	objs *bpfmodule.XMRunQLatObjects
}

const _runqLatMapKey int32 = -1

var _buckets = [20]float64{1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535, 131071, 262143, 524287, 1048575}

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
	// init prometheus section
	rqlM.runqLatHistogramDesc = prometheus.NewDesc(
		prometheus.BuildFQName("process", "schedule", "runq_latency_usecs"),
		"A histogram of the a task spends waiting on a run queue for a turn on-CPU durations.",
		nil, nil,
	)
	rqlM.buckets = make(map[float64]uint64, len(_buckets))

	interval := config.EBPFModuleInterval(name)

	eBPFTracing := func() {
		glog.Infof("eBPFModule:'%s' start gathering eBPF data...", rqlM.name)
		rqlM.gatherTimer.Reset(interval)

		var histogram bpfmodule.XMRunQLatXmRunqlatHist

	loop:
		for {
			select {
			case <-rqlM.stopChan:
				glog.Infof("eBPFModule:'%s' receive stop notify", rqlM.name)
				break loop
			case <-rqlM.gatherTimer.Chan():
				// gather eBPF data
				if err := rqlM.objs.XmRunqlatHistsMap.Lookup(_runqLatMapKey, &histogram); err != nil {
					if !errors.Is(err, ebpf.ErrKeyNotExist) {
						glog.Errorf("eBPFModule:'%s' Lookup error. err:%s", rqlM.name, err.Error())
					}
				} else {
					// glog.Infof("eBPFModule:'%s' runqLat histogram:%#+v", rqlM.name, histogram)

					if err := rqlM.objs.XmRunqlatHistsMap.Delete(_runqLatMapKey); err != nil {
						glog.Errorf("eBPFModule:'%s' Delete error. err:%s", rqlM.name, err.Error())
					}

					// 统计
					rqlM.mu.Lock()
					rqlM.sampleCount = 0
					rqlM.sampleSum = 0.0
					glog.Infof("eBPFModule:'%s' ===>", rqlM.name)
					for i, slot := range histogram.Slots {
						bucket := _buckets[i]
						// 统计本周期的样本总数
						rqlM.sampleCount += uint64(slot)
						// 估算样本的总和
						rqlM.sampleSum += float64(slot) * bucket * 0.6
						// 每个桶的样本数
						rqlM.buckets[bucket] = rqlM.sampleCount
						glog.Infof("\tusecs(%d -> %d) count: %d", func() int {
							if i == 0 {
								return 0
							} else {
								return int(_buckets[i-1])
							}
						}(), int(bucket), slot)
					}
					rqlM.mu.Unlock()
					glog.Infof("eBPFModule:'%s' <===", rqlM.name)
				}

				rqlM.gatherTimer.Reset(interval)
			}
		}
	}

	rqlM.wg.Go(eBPFTracing)

	return rqlM, nil
}

func (rqlM *runqLatModule) Update(ch chan<- prometheus.Metric) error {
	rqlM.mu.Lock()
	defer rqlM.mu.Unlock()
	ch <- prometheus.MustNewConstHistogram(rqlM.runqLatHistogramDesc,
		rqlM.sampleCount, rqlM.sampleSum, rqlM.buckets)
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
