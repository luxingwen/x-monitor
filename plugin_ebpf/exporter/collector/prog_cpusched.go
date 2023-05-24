/*
 * @Author: CALM.WU
 * @Date: 2023-03-25 12:43:28
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-05-18 14:34:08
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
	"github.com/emirpasic/gods/sets/hashset"
	"github.com/golang/glog"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/eventcenter"
)

type cpuSchedProgRodata struct {
	config.ProgRodataBase
	OffCPUMinDurationMillsecs int `mapstructure:"offcpu_min_millsecs"` // offcpu状态的持续时间阈值，毫秒
	OffCPUMaxDurationMillsecs int `mapstructure:"offcpu_max_millsecs"` // offcpu状态的持续时间阈值，毫秒
	OffCPUTaskType            int `mapstructure:"offcpu_task_type"`    // offcpu状态的任务类型
}

type cpuSchedProgram struct {
	scEvtRD             *ringbuf.Reader
	cpuSchedEvtDataChan *bpfmodule.CpuSchedEvtDataChannel
	metricsUpdateFilter *hashset.Set
	runQueueLatencyDesc *prometheus.Desc
	offCPUDurationDesc  *prometheus.Desc
	hangProcessDesc     *prometheus.Desc
	*eBPFBaseProgram
	objs               *bpfmodule.XMCpuScheduleObjects
	runQLatencyBuckets map[float64]uint64
	sampleCount        uint64
	sampleSum          float64
	gatherInterval     time.Duration
	guard              sync.Mutex
}

const (
	defaultCpuSchedEvtChanSize       = (1 << 8)
	defaultRunQueueLatMapKey   int32 = -1
)

var (
	roData cpuSchedProgRodata
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
			"__filter_scope_type":            int32(roData.FilterScopeType),
			"__filter_scope_value":           int64(roData.FilterScopeValue),
			"__offcpu_min_duration_nanosecs": int64(time.Duration(roData.OffCPUMinDurationMillsecs) * time.Millisecond),
			"__offcpu_max_duration_nanosecs": int64(time.Duration(roData.OffCPUMaxDurationMillsecs) * time.Millisecond),
			"__offcpu_task_type":             int8(roData.OffCPUTaskType),
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
		gatherInterval:      config.ProgramConfigByName(name).GatherInterval * time.Second,
		metricsUpdateFilter: hashset.New(),
		runQLatencyBuckets:  make(map[float64]uint64, len(runQueueLayBucketLimits)),
		cpuSchedEvtDataChan: bpfmodule.NewCpuSchedEvtDataChannel(defaultCpuSchedEvtChanSize),
	}

	mapstructure.Decode(config.ProgramConfigByName(name).Filter.ProgRodata, &roData)
	glog.Infof("eBPFProgram:'%s' roData:%s", name, litter.Sdump(roData))

	for _, metricDesc := range config.ProgramConfigByName(name).Metrics {
		switch metricDesc {
		case "runq_latency":
			csProg.runQueueLatencyDesc = prometheus.NewDesc(
				prometheus.BuildFQName("cpu", "schedule", metricDesc),
				"This measures the time a task spends waiting on a run queue for a turn on-CPU, and shows this time as a histogram. in microseconds.",
				[]string{"res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"},
			)
		case "offcpu_duration":
			csProg.offCPUDurationDesc = prometheus.NewDesc(
				prometheus.BuildFQName("cpu", "schedule", metricDesc),
				"The duration of the Task's off-CPU state, in milliseconds.",
				[]string{"pid", "tgid", "comm", "res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"},
			)
		case "hang_process":
			csProg.hangProcessDesc = prometheus.NewDesc(
				prometheus.BuildFQName("cpu", "schedule", metricDesc),
				"pid of the process that is hung",
				[]string{"tgid", "comm", "res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"},
			)
		}
	}

	if err := loadToRunCPUSchedEBPFProg(name, csProg); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' runCpuSchedProgram failed.", name)
		glog.Error(err)
		return nil, err
	}

	csProg.wg.Go(csProg.tracingeBPFEvent)
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
			return roData.FilterScopeType.String()
		}(),
		func() string {
			return strconv.Itoa(roData.FilterScopeValue)
		}(),
	)
	csp.guard.Unlock()

	// 将cpu schedule event数据转换为Prometheus指标
L:
	for {
		select {
		case cpuSchedEvtData, ok := <-csp.cpuSchedEvtDataChan.C:
			if ok {
				//glog.Infof("eBPFProgram:'%s' cpuSchedEvtData:%s", csp.name, litter.Sdump(cpuSchedEvtData))
				if cpuSchedEvtData.EvtType == bpfmodule.XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_OFFCPU {
					if !csp.metricsUpdateFilter.Contains(cpuSchedEvtData.Pid) {
						ch <- prometheus.MustNewConstMetric(csp.offCPUDurationDesc,
							prometheus.GaugeValue, float64(cpuSchedEvtData.OffcpuDurationMillsecs),
							strconv.FormatInt(int64(cpuSchedEvtData.Pid), 10),
							strconv.FormatInt(int64(cpuSchedEvtData.Tgid), 10),
							internal.CommToString(cpuSchedEvtData.Comm[:]),
							roData.FilterScopeType.String(),
							strconv.Itoa(roData.FilterScopeValue))
						csp.metricsUpdateFilter.Add(cpuSchedEvtData.Pid)
					}
				} else if cpuSchedEvtData.EvtType == bpfmodule.XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_HANG {
					ch <- prometheus.MustNewConstMetric(csp.hangProcessDesc,
						prometheus.GaugeValue, float64(cpuSchedEvtData.Pid),
						strconv.FormatInt(int64(cpuSchedEvtData.Tgid), 10),
						internal.CommToString(cpuSchedEvtData.Comm[:]),
						roData.FilterScopeType.String(),
						strconv.Itoa(roData.FilterScopeValue))
				} else if cpuSchedEvtData.EvtType == bpfmodule.XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_PROCESS_EXIT {
					evtInfo := new(eventcenter.EBPFEventInfo)
					evtInfo.EvtType = eventcenter.EBPF_EVENT_PROCESS_EXIT
					evtInfo.EvtData = &eventcenter.EBPFEventProcessExit{
						Pid:  cpuSchedEvtData.Pid,
						Tgid: cpuSchedEvtData.Tgid,
						Comm: internal.CommToString(cpuSchedEvtData.Comm[:]),
					}
					eventcenter.DefInstance.Publish(csp.name, evtInfo)
				}
			}
		default:
			break L
		}
	}

	csp.metricsUpdateFilter.Clear()
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
							return int(runQueueLayBucketLimits[i-1]) + 1
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

func (csp *cpuSchedProgram) tracingeBPFEvent() {
	glog.Infof("eBPFProgram:'%s' start tracing eBPF Event...", csp.name)

loop:
	for {
		record, err := csp.scEvtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningf("eBPFProgram:'%s' tracing eBPF Event goroutine receive stop notify", csp.name)
				break loop
			}
			glog.Errorf("eBPFProgram:'%s' Read error. err:%s", csp.name, err.Error())
			continue
		}

		// 解析数据
		scEvtData := new(bpfmodule.XMCpuScheduleXmCpuSchedEvtData)
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, scEvtData); err != nil {
			glog.Errorf("failed to parse XMCpuScheduleXmCpuSchedEvtData, err: %v", err)
			continue
		}

		comm := internal.CommToString(scEvtData.Comm[:])
		if config.ProgramCommFilter(csp.name, comm) {
			glog.Infof("eBPFProgram:'%s' tgid:%d, pid:%d, comm:'%s', offcpu_duration_millsecs:%d",
				csp.name, scEvtData.Tgid, scEvtData.Pid, comm, scEvtData.OffcpuDurationMillsecs)
			csp.cpuSchedEvtDataChan.SafeSend(scEvtData, false)
		}
	}
}
