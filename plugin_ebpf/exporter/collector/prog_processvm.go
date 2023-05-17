/*
 * @Author: CALM.WU
 * @Date: 2023-04-27 14:10:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-27 15:21:07
 */

package collector

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/emirpasic/gods/maps/treemap"
	"github.com/emirpasic/gods/utils"
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

const (
	defaultProcessVMEvtChanSize = ((1 << 3) << 10)
)

// processVMMProgRodata is used to filter the resources of the specified type
// and value in the rodata section of the process vmmap
type processVMMProgRodata struct {
	config.ProgRodataBase
}

type processVM struct {
	pid      int32
	comm     string
	brkSize  uint64 // brk扩展的地址空间大小
	mmapSize uint64 // mmap私有匿名映射 + file映射的空间大小
}

type processVMProgram struct {
	*eBPFBaseProgram
	// self config
	gatherInterval time.Duration
	// ebpf
	vmmEvtRD *ringbuf.Reader
	objs     *bpfmodule.XMProcessVMObjects
	// manager
	processVMMap         *treemap.Map
	processVMExportGuard sync.Mutex
	// channel
	processVMDataEvtChan *bpfmodule.ProcessVMEvtDataChannel
	// prometheus metric desc
	addressSpaceExpandDesc *prometheus.Desc
}

var (
	pvmRoData processVMMProgRodata
)

func init() {
	registerEBPFProgram(processVMMProgName, newProcessVMMProgram)
}

func loadToRunProcessVMMProg(name string, prog *processVMProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMProcessVMObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMProcessVM, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_scope_type":  int32(pvmRoData.FilterScopeType),
			"__filter_scope_value": int64(pvmRoData.FilterScopeValue),
		})

		if err != nil {
			return errors.Wrapf(err, "eBPFProgram:'%s'", name)
		}
		return nil
	})

	if err != nil {
		prog.objs.Close()
		prog.objs = nil
		return err
	}

	prog.vmmEvtRD, err = ringbuf.NewReader(prog.objs.XmProcessvmEventRingbufMap)
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

func newProcessVMMProgram(name string) (eBPFProgram, error) {
	prog := &processVMProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
		gatherInterval:       config.ProgramConfigByName(name).GatherInterval * time.Second,
		processVMMap:         treemap.NewWith(utils.Int32Comparator),
		processVMDataEvtChan: bpfmodule.NewProcessVMEvtDataChannel(defaultProcessVMEvtChanSize),
	}

	mapstructure.Decode(config.ProgramConfigByName(name).Filter.ProgRodata, &pvmRoData)
	glog.Infof("eBPFProgram:'%s' roData:%s", name, litter.Sdump(pvmRoData))

	for _, metricDesc := range config.ProgramConfigByName(name).Metrics {
		switch metricDesc {
		case "expand_pages":
			prog.addressSpaceExpandDesc = prometheus.NewDesc(
				prometheus.BuildFQName("process", "address_space", metricDesc),
				fmt.Sprintf("Within %d seconds, the process uses mmap or brk to expand the size of private anonymous or shared or heap address spaces, measured in PageSize.",
					int(prog.gatherInterval/time.Second)),
				[]string{"tgid", "comm", "call", "res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"},
			)
		}
	}

	if err := loadToRunProcessVMMProg(name, prog); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' runProcessVMMProgram failed.", name)
		glog.Error(err)
		return nil, err
	}

	prog.wg.Go(prog.tracingeBPFEvent)
	prog.wg.Go(prog.handingeBPFData)

	return prog, nil
}

func (pvp *processVMProgram) tracingeBPFEvent() {
	glog.Infof("eBPFProgram:'%s' start tracing eBPF Event...", pvp.name)

loop:
	for {
		record, err := pvp.vmmEvtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningf("eBPFProgram:'%s' tracing eBPF Event goroutine receive stop notify", pvp.name)
				break loop
			}
			glog.Errorf("eBPFProgram:'%s' Read error. err:%s", pvp.name, err.Error())
			continue
		}

		processVMMEvtData := new(bpfmodule.XMProcessVMXmProcessvmEvtData)
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, processVMMEvtData); err != nil {
			glog.Errorf("failed to parse XMProcessVMMXmVMMEvtData, err: %v", err)
			continue
		}

		pvp.processVMDataEvtChan.SafeSend(processVMMEvtData, false)
	}
}

func (pvp *processVMProgram) handingeBPFData() {
	glog.Infof("eBPFProgram:'%s' start handingeBPFData processVM event data...", pvp.name)

	pvp.gatherTimer.Reset(pvp.gatherInterval)

loop:
	for {
		select {
		case <-pvp.stopChan:
			glog.Warningf("eBPFProgram:'%s' handingeBPFData processVM event receive stop notify", pvp.name)
			break loop
		case data, ok := <-pvp.processVMDataEvtChan.C:
			if ok {
				comm := internal.CommToString(data.Comm[:])
				if config.ProgramCommFilter(pvp.name, comm) {

					pvp.processVMExportGuard.Lock()

					opStr, _ := strings.CutPrefix(data.EvtType.String(), "XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_")

					if pvmI, ok := pvp.processVMMap.Get(data.Tgid); !ok {
						pvm := new(processVM)
						pvm.pid = data.Tgid
						switch data.EvtType {
						case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV:
							fallthrough
						case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED:
							fallthrough
						case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_OTHER:
							pvm.mmapSize = data.Len
						case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK:
							pvm.brkSize = data.Len
						}
						pvm.comm = comm
						pvp.processVMMap.Put(pvm.pid, pvm)
						glog.Infof("eBPFProgram:'%s', count:%d, new comm:'%s', pid:%d, mmapSize:%d, brkSize:%d, '%s':%d bytes in VM",
							pvp.name, pvp.processVMMap.Size(), pvm.comm, pvm.pid, pvm.mmapSize, pvm.brkSize, opStr, data.Len)
					} else {
						if pvm, ok := pvmI.(*processVM); ok {
							if pvm.comm == comm {
								switch data.EvtType {
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV:
									fallthrough
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED:
									fallthrough
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_OTHER:
									pvm.mmapSize += data.Len
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK:
									pvm.brkSize += data.Len
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK_SHRINK:
									if pvm.brkSize >= data.Len {
										pvm.brkSize -= data.Len
									}
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MUNMAP:
									if pvm.mmapSize >= data.Len {
										pvm.mmapSize -= data.Len
									}
								}
								glog.Infof("eBPFProgram:'%s', count:%d, comm:'%s', pid:%d, mmapSize:%d, brkSize:%d, '%s':%d bytes in VM",
									pvp.name, pvp.processVMMap.Size(), pvm.comm, pvm.pid, pvm.mmapSize, pvm.brkSize, opStr, data.Len)
							} else {
								// 进程不同
								prevComm := pvm.comm
								pvp.processVMMap.Remove(data.Tgid)
								pvm := new(processVM)
								pvm.pid = data.Tgid
								switch data.EvtType {
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV:
									fallthrough
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED:
									fallthrough
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_OTHER:
									pvm.mmapSize = data.Len
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK:
									pvm.brkSize = data.Len
								}
								pvm.comm = comm
								pvp.processVMMap.Put(pvm.pid, pvm)
								glog.Infof("eBPFProgram:'%s', count:%d, change prev:'%s', comm:'%s', pid:%d, mmapSize:%d, brkSize:%d, '%s':%d bytes in VM",
									pvp.name, pvp.processVMMap.Size(), prevComm, pvm.comm, pvm.pid, pvm.mmapSize, pvm.brkSize, opStr, data.Len)
							}
						}
					}
					pvp.processVMExportGuard.Unlock()
				}
			}
		case <-pvp.gatherTimer.Chan():
			pvp.processVMExportGuard.Lock()
			pvp.processVMMap.Clear()
			glog.Infof("eBPFProgram:'%s' clean up all processVMs!", pvp.name)
			pvp.processVMExportGuard.Unlock()
			pvp.gatherTimer.Reset(pvp.gatherInterval)
		}
	}
}

func (pvp *processVMProgram) Update(ch chan<- prometheus.Metric) error {
	count := 0
	maxExportCount := config.ProgramConfigByName(pvp.name).Filter.ObjectCount

	pvp.processVMExportGuard.Lock()
	processVMs := make([]*processVM, pvp.processVMMap.Size())

	iter := pvp.processVMMap.Iterator()
	iter.Begin()
	for iter.Next() {
		if pvm, ok := iter.Value().(*processVM); ok {
			processVMs[count] = pvm
			count++
		}
	}

	// 排序
	sort.Slice(processVMs, func(i, j int) bool {
		return (processVMs[i].brkSize + processVMs[i].mmapSize) > (processVMs[j].brkSize + processVMs[j].mmapSize)
	})

	for i := 0; i < count && i < maxExportCount; i++ {
		pvm := processVMs[i]
		// glog.Infof("eBPFProgram:'%s' pid:%d, comm:'%s', mmapSize:%d, brkSize:%d",
		// 	pvp.name, pvm.pid, pvm.comm, pvm.mmapSize, pvm.brkSize)
		pidStr := strconv.FormatInt(int64(pvm.pid), 10)
		ch <- prometheus.MustNewConstMetric(pvp.addressSpaceExpandDesc,
			prometheus.GaugeValue, float64(pvm.mmapSize>>12),
			pidStr, pvm.comm, "mmap", roData.FilterScopeType.String(), strconv.Itoa(roData.FilterScopeValue))
		ch <- prometheus.MustNewConstMetric(pvp.addressSpaceExpandDesc,
			prometheus.GaugeValue, float64(pvm.brkSize>>12),
			pidStr, pvm.comm, "brk", roData.FilterScopeType.String(), strconv.Itoa(roData.FilterScopeValue))
	}

	pvp.processVMExportGuard.Unlock()

	// 释放
	processVMs = nil
	return nil
}

func (pvp *processVMProgram) Stop() {
	if pvp.vmmEvtRD != nil {
		pvp.vmmEvtRD.Close()
	}

	pvp.stop()

	if pvp.objs != nil {
		pvp.objs.Close()
		pvp.objs = nil
	}

	pvp.processVMDataEvtChan.SafeClose()
	glog.Infof("eBPFProgram:'%s' stopped", pvp.name)
}

// getEvtName takes in an XMProcessVMMXmVmmEvtType and returns the name of the event. If the event type is not found, "Unknown" is returned.
func (pvp *processVMProgram) getEvtName(data bpfmodule.XMProcessVMXmProcessvmEvtType) string {
	evtStr := data.String()
	evtName, exist := strings.CutPrefix(evtStr, "XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_")
	if exist {
		return evtName
	}
	return "Unknown"
}
