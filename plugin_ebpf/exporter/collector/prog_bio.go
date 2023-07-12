/*
 * @Author: CALM.WU
 * @Date: 2023-07-10 14:15:12
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-07-10 15:50:27
 */

package collector

import (
	"sync"

	"github.com/cilium/ebpf"
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
)

type bioProgRodata struct {
	FilterPerCmdFlag bool `mapstructure:"filter_per_cmd_flag"`
}

type bioProgram struct {
	*eBPFBaseProgram
	objs  *bpfmodule.XMBioObjects
	guard sync.Mutex

	bioRequestCompletedNumberDesc   *prometheus.Desc // bio request完成的数量
	bioRequestSequentialPercentDesc *prometheus.Desc // bio request顺序完成的数量
	bioRequestRandomizedPercentDesc *prometheus.Desc // bio request随机完成的数量
	bioRequestBytesTotal            *prometheus.Desc // bio request总的字节数，单位kB
	bioRequestIn2CLatency           *prometheus.Desc // bio request从insert到complete的时间，单位us
	bioRequestIs2CLatency           *prometheus.Desc // bio request从issue到complete的时间，单位us
}

var (
	bioRoData bioProgRodata
)

func init() {
	registerEBPFProgram(bioProgName, newBIOProgram)
}

func loadToRunBIOProg(name string, prog *bioProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMBioObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMBio, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_per_cmd_flag": bioRoData.FilterPerCmdFlag,
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

	return err
}

func newBIOProgram(name string) (eBPFProgram, error) {
	bioProg := &bioProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:     name,
			stopChan: make(chan struct{}),
		},
	}

	bioProg.bioRequestCompletedNumberDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "completed_number"),
		"Number of Completed BIO Requests.",
		[]string{"dev_name"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestSequentialPercentDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "sequential_percent"),
		"Percentage of Sequentially Completed BIO Requests.",
		[]string{"dev_name"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestRandomizedPercentDesc = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "randomized_percent"),
		"Percentage of Randomized Completed BIO Requests.",
		[]string{"dev_name"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestBytesTotal = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "total_kbytes"),
		"Total KBytes in BIO Requests.",
		[]string{"dev_name"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestIn2CLatency = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "in2c_latency"),
		"Latency from BIO Request Insert to Complete.",
		[]string{"dev_name"}, prometheus.Labels{"from": "xm_ebpf"})

	bioProg.bioRequestIs2CLatency = prometheus.NewDesc(prometheus.BuildFQName("bio", "request", "is2c_latency"),
		"Latency from BIO Request Issue to Complete.",
		[]string{"dev_name"}, prometheus.Labels{"from": "xm_ebpf"})

	mapstructure.Decode(config.ProgramConfigByName(name).Filter.ProgRodata, &bioRoData)
	glog.Infof("eBPFProgram:'%s' bioRoData:%s", name, litter.Sdump(bioRoData))

	if err := loadToRunBIOProg(name, bioProg); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' load and run failed.", name)
		glog.Error(err)
		return nil, err
	}

	return bioProg, nil
}

func (bp *bioProgram) Update(ch chan<- prometheus.Metric) error {
	var bioInfoMapKey bpfmodule.XMBioXmBioKey
	var bioInfoMapData bpfmodule.XMBioXmBioData
	var dev, sampleCount uint64
	sampleSum := 0.0

	keys := make([]bpfmodule.XMBioXmBioKey, 0)
	latencyBuckets := make(map[float64]uint64, len(__powerOfTwo))

	partitions, _ := calmutils.ProcPartitions()
	//glog.Infof("eBPFProgram:'%s' partitions:%#v", bp.name, partitions)

	entries := bp.objs.XmBioInfoMap.Iterate()
	for entries.Next(&bioInfoMapKey, &bioInfoMapData) {
		keys = append(keys, bioInfoMapKey)
		// 开始计算bio采集数据
		dev = unix.Mkdev(uint32(bioInfoMapKey.Major), uint32(bioInfoMapKey.FirstMinor))
		// 查找设备名
		devName := "Unknown"
		for _, p := range partitions {
			if p.Dev == dev {
				devName = p.DevName
			}
		}

		total := bioInfoMapData.SequentialCount + bioInfoMapData.RandomCount
		// 随机操作的百分比
		random_percent := uint32(float32(bioInfoMapData.RandomCount) / float32(total) * 100.0)
		// 顺序操作的字节数
		sequential_percent := uint32(float32(bioInfoMapData.SequentialCount) / float32(total) * 100.0)
		// 操作完成的字节数
		kBytes := (bioInfoMapData.Bytes >> 10)

		glog.Infof("eBPFProgram:'%s' dev:'%d', devName:'%s', random:%d%%, sequential:%d%%, bytes:%dKB",
			bp.name, dev, devName, random_percent, sequential_percent, kBytes)

		ch <- prometheus.MustNewConstMetric(bp.bioRequestCompletedNumberDesc, prometheus.GaugeValue, float64(total), devName)
		ch <- prometheus.MustNewConstMetric(bp.bioRequestSequentialPercentDesc, prometheus.GaugeValue, float64(sequential_percent), devName)
		ch <- prometheus.MustNewConstMetric(bp.bioRequestRandomizedPercentDesc, prometheus.GaugeValue, float64(random_percent), devName)
		ch <- prometheus.MustNewConstMetric(bp.bioRequestBytesTotal, prometheus.GaugeValue, float64(kBytes), devName)

		for i, slot := range bioInfoMapData.ReqLatencyIn2cSlots {
			bucket := float64(__powerOfTwo[i])        // 桶的上限
			sampleCount += uint64(slot)               // 统计本周期的样本总数
			sampleSum += float64(slot) * bucket * 0.6 // 估算样本的总和
			latencyBuckets[bucket] = sampleCount      // 每个桶的样本数，下层包括上层统计数量
			glog.Infof("\tbioRequestIn2C usecs(%d -> %d) count: %d", func() int {
				if i == 0 {
					return 0
				} else {
					return int(__powerOfTwo[i-1]) + 1
				}
			}(), int(bucket), slot)
		}

		ch <- prometheus.MustNewConstHistogram(bp.bioRequestIn2CLatency,
			sampleCount, sampleSum, maps.Clone(latencyBuckets), devName)

		maps.Clear(latencyBuckets)

		for i, slot := range bioInfoMapData.ReqLatencyIs2cSlots {
			bucket := float64(__powerOfTwo[i])        // 桶的上限
			sampleCount += uint64(slot)               // 统计本周期的样本总数
			sampleSum += float64(slot) * bucket * 0.6 // 估算样本的总和
			latencyBuckets[bucket] = sampleCount      // 每个桶的样本数，下层包括上层统计数量
			glog.Infof("\tbioRequestIs2C usecs(%d -> %d) count: %d", func() int {
				if i == 0 {
					return 0
				} else {
					return int(__powerOfTwo[i-1]) + 1
				}
			}(), int(bucket), slot)
		}

		ch <- prometheus.MustNewConstHistogram(bp.bioRequestIs2CLatency,
			sampleCount, sampleSum, maps.Clone(latencyBuckets), devName)

		maps.Clear(latencyBuckets)
	}

	if err := entries.Err(); err != nil {
		glog.Errorf("eBPFProgram:'%s' iterator BioInfoMap failed, err:%s", err.Error())
	}

	// 删除XmBioInfoMap的所有数据
	count, err := bp.objs.XmBioInfoMap.BatchDelete(keys, nil)
	if err != nil {
		glog.Errorf("eBPFProgram:'%s' BatchDelete XmBioInfoMap failed, err:%s", bp.name, err.Error())
	} else {
		glog.Infof("eBPFProgram:'%s' BatchDelete XmBioInfoMap count:%d", bp.name, count)
	}

	return nil
}

func (bp *bioProgram) Stop() {
	bp.stop()

	if bp.objs != nil {
		bp.objs.Close()
		bp.objs = nil
	}
	glog.Infof("eBPFProgram:'%s' stopped", bp.name)
}
