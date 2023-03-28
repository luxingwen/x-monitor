/*
 * @Author: CALM.WU
 * @Date: 2023-03-25 12:43:28
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-28 18:27:11
 */

package collector

import (
	"bytes"
	"encoding/binary"
	"strconv"
	"sync"
	"time"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/golang/glog"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal"
)

type cpuSchedProgRodata struct {
	FilterScopeType         config.XMInternalResourceType `mapstructure:"filter_scope_type"`         // 过滤的资源类型
	FilterScopeValue        int                           `mapstructure:"filter_scope_value"`        // 过滤资源的值
	OffCPUThresholdMillsecs int                           `mapstructure:"offcpu_threshold_millsecs"` // offcpu状态的持续时间阈值，毫秒
	OffCPUTaskType          int                           `mapstructure:"offcpu_task_type"`          // offcpu状态的任务类型
}

type cpuSchedProgram struct {
	*eBPFBaseProgram
	roData         cpuSchedProgRodata
	gatherInterval time.Duration

	// prometheus metric desc
	runQueueLatencyDesc *prometheus.Desc
	offCPUOverTimeDesc  *prometheus.Desc
	hangProcessDesc     *prometheus.Desc

	sampleCount        uint64
	sampleSum          float64
	runQLatencyBuckets map[float64]uint64
	guard              sync.Mutex
	// ebpf ringbuf map fd
	scEvtRD *ringbuf.Reader

	// eBPF对象
	objs *bpfmodule.XMCpuScheduleObjects

	// cpu sched evt data channel
	cpuSchedEvtDataChan *bpfmodule.CpuSchedEvtDataChannel
}

const (
	defaultCpuSchedEvtChanSize       = 256
	defaultRunQueueLatMapKey   int32 = -1
)

var runQueueLayBucketLimits = [20]float64{1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535, 131071, 262143, 524287, 1048575}

func init() {
	registerEBPFProgram(cpuSchedProgName, newCpuSchedProgram)
}

func loadToRunCPUSchedEBPFProg(name string, prog *cpuSchedProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMCpuScheduleObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMCpuSchedule, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_scope_type":         int32(prog.roData.FilterScopeType),
			"__filter_scope_value":        int64(prog.roData.FilterScopeValue),
			"__offcpu_threshold_nanosecs": int64(time.Duration(prog.roData.OffCPUThresholdMillsecs) * time.Millisecond),
			"__offcpu_task_type":          int8(prog.roData.OffCPUTaskType),
		})

		if err != nil {
			return errors.Wrap(err, "RewriteConstants failed.")
		}
		return err
	})

	if err != nil {
		prog.objs.Close()
		prog.objs = nil
		return err
	}

	prog.scEvtRD, err = ringbuf.NewReader(prog.objs.XmCsEventRingbufMap)
	if err != nil {
		for _, link := range prog.links {
			link.Close()
		}
		prog.links = nil
		prog.objs.Close()
		prog.objs = nil
	}
	return err
}

func newCpuSchedProgram(name string) (eBPFProgram, error) {
	csProg := &cpuSchedProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
		gatherInterval:      config.ProgramConfig(name).GatherInterval * time.Second,
		runQLatencyBuckets:  make(map[float64]uint64, len(runQueueLayBucketLimits)),
		cpuSchedEvtDataChan: bpfmodule.NewCpuSchedEvtDataChannel(defaultCpuSchedEvtChanSize),
	}

	mapstructure.Decode(config.ProgramConfig(name).ProgRodata, &csProg.roData)
	glog.Infof("eBPFProgram:'%s' roData:%s", name, litter.Sdump(csProg.roData))

	for _, metricDesc := range config.ProgramConfig(name).Metrics {
		switch metricDesc {
		case "runq_latency_usecs":
			csProg.runQueueLatencyDesc = prometheus.NewDesc(
				prometheus.BuildFQName("cpu", "schedule", metricDesc),
				"A histogram of the a task spends waiting on a attatchToRun queue for a turn on-CPU durations.",
				[]string{"res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"},
			)
		case "offcpu_over_time":
			csProg.offCPUOverTimeDesc = prometheus.NewDesc(
				prometheus.BuildFQName("cpu", "schedule", metricDesc),
				"The off-CPU delay time of tasks that exceed the threshold.",
				[]string{"pid", "tgid", "res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"},
			)
		case "hang_process":
			csProg.hangProcessDesc = prometheus.NewDesc(
				prometheus.BuildFQName("cpu", "schedule", metricDesc),
				"Display processes that are hung in the process scheduler.",
				[]string{"pid", "tgid", "res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"},
			)
		}
	}

	if err := loadToRunCPUSchedEBPFProg(name, csProg); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' runCpuSchedProgram failed.", name)
		glog.Error(err)
		return nil, err
	}

	csProg.wg.Go(csProg.tracingCSEvent)
	csProg.wg.Go(csProg.tracingRunQLayency)

	return csProg, nil
}

func (csp *cpuSchedProgram) Update(ch chan<- prometheus.Metric) error {
	csp.guard.Lock()
	// avoid data race
	buckets := make(map[float64]uint64)
	for k, v := range csp.runQLatencyBuckets {
		buckets[k] = v
	}

	ch <- prometheus.MustNewConstHistogram(csp.runQueueLatencyDesc,
		csp.sampleCount, csp.sampleSum, buckets,
		func() string {
			return csp.roData.FilterScopeType.String()
		}(),
		func() string {
			return strconv.Itoa(csp.roData.FilterScopeValue)
		}(),
	)
	csp.guard.Unlock()

	// 将cpu schedule event数据转换为Prometheus指标
L:
	for {
		select {
		case cpuSchedEvtData, ok := <-csp.cpuSchedEvtDataChan.C:
			if ok {
				glog.Infof("eBPFProgram:'%s' cpuSchedEvtData:%s", csp.name, litter.Sdump(cpuSchedEvtData))
			}
		default:
			break L
		}
	}
	return nil
}

func (csp *cpuSchedProgram) Stop() {
	if csp.scEvtRD != nil {
		// 关闭ringbuf
		csp.scEvtRD.Close()
	}

	csp.stop()

	if csp.objs != nil {
		csp.objs.Close()
		csp.objs = nil
	}
	csp.cpuSchedEvtDataChan.SafeClose()
	glog.Infof("eBPFProgram:'%s' stopped.", csp.name)
}

func (csp *cpuSchedProgram) tracingRunQLayency() {
	glog.Infof("eBPFProgram:'%s' start tracing runQueueLatency data...", csp.name)
	csp.gatherTimer.Reset(csp.gatherInterval)

	var histogram bpfmodule.XMCpuScheduleXmRunqlatHist

loop:
	for {
		select {
		case <-csp.stopChan:
			glog.Warningf("eBPFProgram:'%s' tracing runQueueLatency receive stop notify", csp.name)
			break loop
		case <-csp.gatherTimer.Chan():
			// gather eBPF data
			if err := csp.objs.XmCsRunqlatHistsMap.Lookup(defaultRunQueueLatMapKey, &histogram); err != nil {
				if !errors.Is(err, ebpf.ErrKeyNotExist) {
					glog.Errorf("eBPFProgram:'%s' Lookup error. err:%s", csp.name, err.Error())
				}
			} else {
				//glog.Infof("eBPFProgram:'%s' runqLat histogram:%#+v", csp.name, histogram)

				if err := csp.objs.XmCsRunqlatHistsMap.Delete(defaultRunQueueLatMapKey); err != nil {
					glog.Errorf("eBPFProgram:'%s' Delete error. err:%s", csp.name, err.Error())
				}

				// 统计
				csp.guard.Lock()
				csp.sampleCount = 0
				csp.sampleSum = 0.0
				glog.Infof("eBPFProgram:'%s' tracing runQueueLatency ===>", csp.name)
				for i, slot := range histogram.Slots {
					bucket := runQueueLayBucketLimits[i]
					csp.sampleCount += uint64(slot)                  // 统计本周期的样本总数
					csp.sampleSum += float64(slot) * bucket * 0.6    // 估算样本的总和
					csp.runQLatencyBuckets[bucket] = csp.sampleCount // 每个桶的样本数，下层包括上层统计数量
					glog.Infof("\tusecs(%d -> %d) count: %d", func() int {
						if i == 0 {
							return 0
						} else {
							return int(_buckets[i-1]) + 1
						}
					}(), int(bucket), slot)
				}
				csp.guard.Unlock()
				glog.Infof("eBPFProgram:'%s' tracing runQueueLatency <===", csp.name)
			}

			csp.gatherTimer.Reset(csp.gatherInterval)
		}
	}
}

func (csp *cpuSchedProgram) tracingCSEvent() {
	glog.Infof("eBPFProgram:'%s' start tracing cpu sched event data...", csp.name)

	var scEvtData bpfmodule.XMCpuScheduleXmCpuSchedEvtData
loop:
	for {
		record, err := csp.scEvtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningf("eBPFProgram:'%s' tracing cpu sched event receive stop notify", csp.name)
				break loop
			}
			glog.Errorf("eBPFProgram:'%s' Read error. err:%s", csp.name, err.Error())
			continue
		}

		// 解析数据
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, &scEvtData); err != nil {
			glog.Errorf("failed to parse XMCpuScheduleXmCpuSchedEvtData, err: %v", err)
			continue
		}

		glog.Infof("eBPFProgram:'%s' tgid:%d, pid:%d, comm:'%s', offcpu_duration_millsecs:%d",
			csp.name, scEvtData.Tgid, scEvtData.Pid, internal.CommToString(scEvtData.Comm[:]), scEvtData.OffcpuDurationMillsecs)
	}
}
