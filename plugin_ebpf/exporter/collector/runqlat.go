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
	"github.com/mitchellh/mapstructure"
	gerr "github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

type SchedObjFilter struct {
	Attribute config.SchedObjAttrType `mapstructure:"sched_obj_filter_attr"`  // 调度对象的过滤属性
	Value     int                     `mapstructure:"sched_obj_filter_value"` // 调度对象过滤属性值
}

type runQLatProgram struct {
	*eBPFBaseProgram
	// 被调度运行对象的过滤条件
	schedObjFilter SchedObjFilter
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
	registerEBPFProgram(runQLatencyProgName, newRunQLatencyProgram)
}

func newRunQLatencyProgram(name string) (eBPFProgram, error) {
	rqlp := &runQLatProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
	}

	interval := config.GatherInterval(name)
	filter := config.Conditions(name)
	mapstructure.Decode(filter, &rqlp.schedObjFilter)
	glog.Infof("eBPFProgram:'%s' gatherInterval:%s, schedObjFilter:%s", name, interval.String(), litter.Sdump(rqlp.schedObjFilter))

	var err error
	rqlp.objs = new(bpfmodule.XMRunQLatObjects)
	rqlp.links, err = attatchToRun(name, rqlp.objs, bpfmodule.LoadXMRunQLat, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_type":  int32(rqlp.schedObjFilter.Attribute),
			"__filter_value": int64(rqlp.schedObjFilter.Value),
		})

		if err != nil {
			return gerr.Wrap(err, "RewriteConstants failed.")
		}
		return nil
	})

	if err != nil {
		rqlp.objs.Close()
		rqlp.objs = nil
		return nil, err
	}
	// init prometheus section
	rqlp.runqLatHistogramDesc = prometheus.NewDesc(
		prometheus.BuildFQName("process", "schedule", "runq_latency_usecs"),
		"A histogram of the a task spends waiting on a attatchToRun queue for a turn on-CPU durations.",
		nil, nil,
	)
	rqlp.buckets = make(map[float64]uint64, len(_buckets))

	eBPFTracing := func() {
		glog.Infof("eBPFProgram:'%s' start gathering eBPF data...", rqlp.name)
		rqlp.gatherTimer.Reset(interval)

		var histogram bpfmodule.XMRunQLatXmRunqlatHist

	loop:
		for {
			select {
			case <-rqlp.stopChan:
				glog.Infof("eBPFProgram:'%s' receive stop notify", rqlp.name)
				break loop
			case <-rqlp.gatherTimer.Chan():
				// gather eBPF data
				if err := rqlp.objs.XmRunqlatHistsMap.Lookup(_runqLatMapKey, &histogram); err != nil {
					if !errors.Is(err, ebpf.ErrKeyNotExist) {
						glog.Errorf("eBPFProgram:'%s' Lookup error. err:%s", rqlp.name, err.Error())
					}
				} else {
					// glog.Infof("eBPFProgram:'%s' runqLat histogram:%#+v", rqlp.name, histogram)

					if err := rqlp.objs.XmRunqlatHistsMap.Delete(_runqLatMapKey); err != nil {
						glog.Errorf("eBPFProgram:'%s' Delete error. err:%s", rqlp.name, err.Error())
					}

					// 统计
					rqlp.mu.Lock()
					rqlp.sampleCount = 0
					rqlp.sampleSum = 0.0
					glog.Infof("eBPFProgram:'%s' ===>", rqlp.name)
					for i, slot := range histogram.Slots {
						bucket := _buckets[i]
						rqlp.sampleCount += uint64(slot)               // 统计本周期的样本总数
						rqlp.sampleSum += float64(slot) * bucket * 0.6 // 估算样本的总和
						rqlp.buckets[bucket] = rqlp.sampleCount        // 每个桶的样本数，下层包括上层统计数量
						glog.Infof("\tusecs(%d -> %d) count: %d", func() int {
							if i == 0 {
								return 0
							} else {
								return int(_buckets[i-1]) + 1
							}
						}(), int(bucket), slot)
					}
					rqlp.mu.Unlock()
					glog.Infof("eBPFProgram:'%s' <===", rqlp.name)
				}

				rqlp.gatherTimer.Reset(interval)
			}
		}
	}

	rqlp.wg.Go(eBPFTracing)

	return rqlp, nil
}

func (rqlp *runQLatProgram) Update(ch chan<- prometheus.Metric) error {
	rqlp.mu.Lock()
	defer rqlp.mu.Unlock()
	ch <- prometheus.MustNewConstHistogram(rqlp.runqLatHistogramDesc,
		rqlp.sampleCount, rqlp.sampleSum, rqlp.buckets)
	return nil
}

// Stop closes the runQLatProgram and stops the eBPF module.
func (rqlp *runQLatProgram) Stop() {
	rqlp.stop()

	if rqlp.objs != nil {
		rqlp.objs.Close()
		rqlp.objs = nil
	}

	glog.Infof("eBPFProgram:'%s' stopped.", rqlp.name)
}
