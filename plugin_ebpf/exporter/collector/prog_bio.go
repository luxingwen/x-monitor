/*
 * @Author: CALM.WU
 * @Date: 2023-07-10 14:15:12
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-07-10 15:50:27
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
	"golang.org/x/exp/maps"
	"golang.org/x/sys/unix"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal"
)

const (
	defaultReqLatencyEvtChanSize = ((1 << 2) << 10)
)

type bioProgRodata struct {
	FilterPerCmdFlag          bool `mapstructure:"filter_per_cmd_flag"`
	RequestMinLatencyMillSecs int  `mapstructure:"request_min_latency_millsecs"`
}

type bioProgram struct {
	*eBPFBaseProgram
	objs                  *bpfmodule.XMBioObjects
	guard                 sync.Mutex
	reqLatencyEvtRD       *ringbuf.Reader
	reqLatencyEvtDataChan *bpfmodule.BioRequestLatencyEvtDataChannel

	bioRequestCompletedCountDesc       *prometheus.Desc // bio request完成的数量
	bioRequestSequentialRatioDesc      *prometheus.Desc // bio request顺序完成的数量
	bioRequestRandomizedRatioDesc      *prometheus.Desc // bio request随机完成的数量
	bioRequestBytesTotalDesc           *prometheus.Desc // bio request总的字节数，单位kB
	bioRequestErrorCountDesc           *prometheus.Desc // bio reqeust失败数量
	bioRequestInQueueLatencyDesc       *prometheus.Desc // bio request在队列的时间，单位us
	bioRequestLatencyDesc              *prometheus.Desc // bio request执行延迟，单位us
	bioRequestLatencyOverThresholdDesc *prometheus.Desc // bio request操作超过了设定的阈值
}

const (
	REQ_OP_READ         = 0
	REQ_OP_WRITE        = 1
	REQ_OP_FLUSH        = 2
	REQ_OP_DISCARD      = 3
	REQ_OP_ZONE_REPORT  = 4
	REQ_OP_SECURE_ERASE = 5
	REQ_OP_ZONE_RESET   = 6
	REQ_OP_WRITE_SAME   = 7
	REQ_OP_WRITE_ZEROES = 9
	REQ_OP_SCSI_IN      = 32
	REQ_OP_SCSI_OUT     = 33
	REQ_OP_DRV_IN       = 34
	REQ_OP_DRV_OUT      = 35
)

var (
	bioRoData            bioProgRodata
	__zeroBioInfoMapData = bpfmodule.XMBioXmBioData{}
	reqOpStrings         = map[uint8]string{
		REQ_OP_READ:         "read",
		REQ_OP_WRITE:        "write",
		REQ_OP_FLUSH:        "flush",
		REQ_OP_DISCARD:      "discard",
		REQ_OP_WRITE_SAME:   "write_same",
		REQ_OP_WRITE_ZEROES: "write_zeroes",
	}
)

func init() {
	registerEBPFProgram(bioProgName, newBIOProgram)
}

func loadToRunBIOProg(name string, prog *bioProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMBioObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMBio, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_per_cmd_flag":    bioRoData.FilterPerCmdFlag,
			"__request_min_latency_ns": int64(time.Duration(bioRoData.RequestMinLatencyMillSecs) * time.Millisecond),
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

	prog.reqLatencyEvtRD, err = ringbuf.NewReader(prog.objs.XmBioReqLatencyEventRingbufMap)
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

func newBIOProgram(name string) (eBPFProgram, error) {
	bioProg := &bioProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:     name,
			stopChan: make(chan struct{}),
		},
		reqLatencyEvtDataChan: bpfmodule.NewBioRequestLatencyEvtDataChannel(defaultReqLatencyEvtChanSize),
	}

	bioProg.bioRequestCompletedCountDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "completed_count"),
		"Count of Completed BIO Requests.",
		[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestSequentialRatioDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "sequential_ratio"),
		"Ratio of Sequentially Completed BIO Requests.",
		[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestRandomizedRatioDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "randomized_ratio"),
		"Ratio of Randomized Completed BIO Requests.",
		[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestBytesTotalDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "total_kbytes"),
		"Number of KBytes in BIO Requests.",
		[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestErrorCountDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "error_count"),
		"Number of failed bio Requests.",
		[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestInQueueLatencyDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "in_queue_latency"),
		"Latency of bio request in queue, unit us.",
		[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestLatencyDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "latency"),
		"Latency of bio request, unit us.",
		[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestLatencyOverThresholdDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "latency_over_threshold"),
		"Latency of bio request over threshold, unit us.", []string{"device", "comm", "pid", "operation", "bytes", "in_queue_latency_us"}, prometheus.Labels{"from": "xm_ebpf"})

	mapstructure.Decode(config.ProgramConfigByName(name).Filter.ProgRodata, &bioRoData)
	glog.Infof("eBPFProgram:'%s' bioRoData:%s", name, litter.Sdump(bioRoData))

	if err := loadToRunBIOProg(name, bioProg); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' load and run failed.", name)
		glog.Error(err)
		return nil, err
	}

	bioProg.wg.Go(bioProg.tracingeBPFEvent)

	return bioProg, nil
}

func (bp *bioProgram) tracingeBPFEvent() {
	glog.Infof("eBPFProgram:'%s' start tracing eBPF Event...", bp.name)

loop:
	for {
		record, err := bp.reqLatencyEvtRD.Read()
		if err != nil {
			if err != nil {
				if errors.Is(err, ringbuf.ErrClosed) {
					glog.Warningf("eBPFProgram:'%s' tracing eBPF Event goroutine receive stop notify", bp.name)
					break loop
				}
				glog.Errorf("eBPFProgram:'%s' Read error. err:%s", bp.name, err.Error())
				continue
			}
		}

		reqLatencyEvtData := new(bpfmodule.XMBioXmBioRequestLatencyEvtData)
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, reqLatencyEvtData); err != nil {
			glog.Errorf("failed to parse XMBioXmBioRequestLatencyEvtData, err: %v", err)
			continue
		}

		bp.reqLatencyEvtDataChan.SafeSend(reqLatencyEvtData, false)
	}
}

func (bp *bioProgram) Update(ch chan<- prometheus.Metric) error {
	var bioInfoMapKey bpfmodule.XMBioXmBioKey
	var bioInfoMapData bpfmodule.XMBioXmBioData
	var dev, sampleCount uint64
	sampleSum := 0.0

	keys := make([]bpfmodule.XMBioXmBioKey, 0)
	latencyBuckets := make(map[float64]uint64, len(__powerOfTwo))

	partitions, _ := calmutils.ProcPartitions()

	pfnFindDevName := func(dev uint64) string {
		// 查找设备名
		devName := "unknown"
		for _, p := range partitions {
			if p.Dev == dev {
				devName = p.DevName
			}
		}
		return devName
	}

	pfnFindOpName := func(cmdFlags uint8) string {
		opName := "all"
		if bioRoData.FilterPerCmdFlag {
			if s, ok := reqOpStrings[cmdFlags]; ok {
				opName = s
			} else {
				opName = "unknown"
			}
		}
		return opName
	}
	//glog.Infof("eBPFProgram:'%s' partitions:%#v", bp.name, partitions)

	entries := bp.objs.XmBioInfoMap.Iterate()
	for entries.Next(&bioInfoMapKey, &bioInfoMapData) {
		keys = append(keys, bioInfoMapKey)
		// 开始计算bio采集数据
		dev = unix.Mkdev(uint32(bioInfoMapKey.Major), uint32(bioInfoMapKey.FirstMinor))
		// 查找设备名
		devName := pfnFindDevName(dev)
		// 操作名
		opName := pfnFindOpName(bioInfoMapKey.CmdFlags)

		total := bioInfoMapData.SequentialCount + bioInfoMapData.RandomCount
		// 随机操作的百分比
		random_ratio := uint32(float32(bioInfoMapData.RandomCount) / float32(total) * 100.0)
		// 顺序操作的字节数
		sequential_ratio := uint32(float32(bioInfoMapData.SequentialCount) / float32(total) * 100.0)
		// 操作完成的字节数
		kBytes := (bioInfoMapData.Bytes >> 10)

		glog.Infof("eBPFProgram:'%s' dev:'%d', devName:'%s', opName:%s(%d), random:%d%%, sequential:%d%%, bytes:%dKB",
			bp.name, dev, devName, opName, bioInfoMapKey.CmdFlags, random_ratio, sequential_ratio, kBytes)

		ch <- prometheus.MustNewConstMetric(bp.bioRequestCompletedCountDesc, prometheus.GaugeValue, float64(total), devName, opName)
		ch <- prometheus.MustNewConstMetric(bp.bioRequestSequentialRatioDesc, prometheus.GaugeValue, float64(sequential_ratio), devName, opName)
		ch <- prometheus.MustNewConstMetric(bp.bioRequestRandomizedRatioDesc, prometheus.GaugeValue, float64(random_ratio), devName, opName)
		ch <- prometheus.MustNewConstMetric(bp.bioRequestBytesTotalDesc, prometheus.GaugeValue, float64(kBytes), devName, opName)
		ch <- prometheus.MustNewConstMetric(bp.bioRequestErrorCountDesc, prometheus.GaugeValue, float64(bioInfoMapData.ReqErrCount), devName, opName)

		for i, slot := range bioInfoMapData.ReqInQueueLatencySlots {
			bucket := float64(__powerOfTwo[i])   // 桶的上限
			sampleCount += uint64(slot)          // 统计本周期的样本总数
			sampleSum += float64(slot) * bucket  // * 0.6 // 估算样本的总和
			latencyBuckets[bucket] = sampleCount // 每个桶的样本数，下层包括上层统计数量
			glog.Infof("\tbio request in queue latency usecs(%d -> %d) count: %d", func() int {
				if i == 0 {
					return 0
				} else {
					return int(__powerOfTwo[i-1]) + 1
				}
			}(), int(bucket), slot)
		}

		ch <- prometheus.MustNewConstHistogram(bp.bioRequestInQueueLatencyDesc,
			sampleCount, sampleSum, maps.Clone(latencyBuckets), devName, opName)

		maps.Clear(latencyBuckets)

		for i, slot := range bioInfoMapData.ReqLatencySlots {
			bucket := float64(__powerOfTwo[i])   // 桶的上限
			sampleCount += uint64(slot)          // 统计本周期的样本总数
			sampleSum += float64(slot) * bucket  // * 0.6 // 估算样本的总和
			latencyBuckets[bucket] = sampleCount // 每个桶的样本数，下层包括上层统计数量
			glog.Infof("\tbio request latency usecs(%d -> %d) count: %d", func() int {
				if i == 0 {
					return 0
				} else {
					return int(__powerOfTwo[i-1]) + 1
				}
			}(), int(bucket), slot)
		}

		ch <- prometheus.MustNewConstHistogram(bp.bioRequestLatencyDesc,
			sampleCount, sampleSum, maps.Clone(latencyBuckets), devName, opName)

		maps.Clear(latencyBuckets)
	}

	if err := entries.Err(); err != nil {
		glog.Errorf("eBPFProgram:'%s' iterator BioInfoMap failed, err:%s", err.Error())
	}

	// 重置数据
	for _, key := range keys {
		// 判断设备是否存在
		exist := func(major, minor uint32) bool {
			for _, p := range partitions {
				if p.Major == major && p.Minor == minor {
					return true
				}
			}
			return false
		}(uint32(key.Major), uint32(key.FirstMinor))

		if exist {
			// update
			if err := bp.objs.XmBioInfoMap.Update(&key, &__zeroBioInfoMapData, ebpf.UpdateExist); err != nil {
				glog.Errorf("eBPFProgram:'%s' Update XmBioInfoMap failed, err:%s", bp.name, err.Error())
			}
		} else {
			// delete
			if err := bp.objs.XmBioInfoMap.Delete(&key); err != nil {
				glog.Errorf("eBPFProgram:'%s' Delete XmBioInfoMap failed, err:%s", bp.name, err.Error())
			}
		}
	}

	// 读取req latency事件
L:
	for {
		select {
		case reqLatencyEvtData, ok := <-bp.reqLatencyEvtDataChan.C:
			if ok {
				comm := internal.CommToString(reqLatencyEvtData.Comm[:])
				// 查找设备名
				devName := pfnFindDevName(unix.Mkdev(uint32(reqLatencyEvtData.Major), uint32(reqLatencyEvtData.FirstMinor)))
				// 操作名
				opName := pfnFindOpName(reqLatencyEvtData.CmdFlags)

				glog.Infof("eBPFProgram:'%s' comm:'%s', pid:%d, tid:%d, devName:'%s', opName:%s, bytes:'%d', in_queue_latency_us:%d, latency_us:%d",
					bp.name, comm, reqLatencyEvtData.Pid, reqLatencyEvtData.Tid,
					devName, opName, reqLatencyEvtData.Len, reqLatencyEvtData.ReqInQueueLatencyUs,
					reqLatencyEvtData.ReqLatencyUs)
				ch <- prometheus.MustNewConstMetric(bp.bioRequestLatencyOverThresholdDesc,
					prometheus.GaugeValue, float64(reqLatencyEvtData.ReqLatencyUs),
					devName, comm,
					strconv.FormatInt(int64(reqLatencyEvtData.Pid), 10),
					opName, strconv.FormatInt(int64(reqLatencyEvtData.Len), 10),
					strconv.FormatInt(int64(reqLatencyEvtData.ReqInQueueLatencyUs), 10))
			}
		default:
			break L
		}
	}

	return nil
}

// Stop stops the bioProgram and closes its objects.
func (bp *bioProgram) Stop() {
	if bp.reqLatencyEvtRD != nil {
		bp.reqLatencyEvtRD.Close()
	}

	bp.stop()

	if bp.objs != nil {
		bp.objs.Close()
		bp.objs = nil
	}

	bp.reqLatencyEvtDataChan.SafeClose()
	glog.Infof("eBPFProgram:'%s' stopped", bp.name)
}
