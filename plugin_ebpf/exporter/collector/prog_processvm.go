/*
 * @Author: CALM.WU
 * @Date: 2023-04-27 14:10:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 16:13:24
 */

package collector

import (
	"bytes"
	"encoding/binary"
	"sort"
	"strconv"
	"strings"
	"sync"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/emirpasic/gods/maps/treemap"
	"github.com/emirpasic/gods/sets/hashset"
	"github.com/emirpasic/gods/utils"
	"github.com/golang/glog"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	bpfprog "xmonitor.calmwu/plugin_ebpf/exporter/internal/bpf_prog"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/eventcenter"
	bpfutils "xmonitor.calmwu/plugin_ebpf/exporter/internal/utils"
)

const (
	defaultProcessVMEvtChanSize = ((1 << 2) << 10)
	pageShift                   = 12
)

type processVMMPrivateArgs struct {
	ObjectCount int `mapstructure:"object_count"`
}

type processVM struct {
	mmapAddrSet *hashset.Set
	comm        string
	brkSize     int64
	mmapSize    int64
	pid         int32
}

type processVMProgram struct {
	*eBPFBaseProgram
	vmmEvtRD               *ringbuf.Reader
	objs                   *bpfmodule.XMProcessVMObjects
	processVMMap           *treemap.Map
	processVMEvtDataChan   *bpfmodule.ProcessVMEvtDataChannel
	addressSpaceExpandDesc *prometheus.Desc
	processVMMapGuard      sync.Mutex
}

var (
	__pvmEBpfArgs    config.EBpfProgBaseArgs
	__pvmPrivateArgs processVMMPrivateArgs
)

func init() {
	registerEBPFProgram(processVMMProgName, newProcessVMMProgram)
}

func loadToRunProcessVMMProg(name string, prog *processVMProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMProcessVMObjects)
	prog.links, err = bpfprog.AttachToRun(name, prog.objs, bpfmodule.LoadXMProcessVM, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_scope_type":  int32(__pvmEBpfArgs.FilterScopeType),
			"__filter_scope_value": int64(__pvmEBpfArgs.FilterScopeValue),
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
			name:     name,
			stopChan: make(chan struct{}),
		},
		processVMMap:         treemap.NewWith(utils.Int32Comparator),
		processVMEvtDataChan: bpfmodule.NewProcessVMEvtDataChannel(defaultProcessVMEvtChanSize),
	}

	mapstructure.Decode(config.ProgramConfigByName(name).Args.EBpfProg, &__pvmEBpfArgs)
	glog.Infof("eBPFProgram:'%s' ebpfProgArgs:%s", name, litter.Sdump(__pvmEBpfArgs))

	for _, metricDesc := range config.ProgramConfigByName(name).Args.Metrics {
		switch metricDesc {
		case "privanon_share_pages":
			prog.addressSpaceExpandDesc = prometheus.NewDesc(
				prometheus.BuildFQName("process", "address_space", metricDesc),
				"In Life Cycle, The process uses mmap or brk to expand the size of private anonymous or shared or heap address spaces, measured in PageSize.",
				[]string{"pid", "comm", "call", "res_type", "res_value"}, prometheus.Labels{"from": "xm_ebpf"})
		}
	}

	if err := loadToRunProcessVMMProg(name, prog); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' runProcessVMMProgram failed.", name)
		glog.Error(err)
		return nil, err
	}

	prog.wg.Go(prog.tracingeBPFEvent)
	prog.wg.Go(prog.handlingeBPFData)

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

		pvp.processVMEvtDataChan.SafeSend(processVMMEvtData, false)
	}
}

func (pvp *processVMProgram) handlingeBPFData() {
	glog.Infof("eBPFProgram:'%s' start handling eBPF Data...", pvp.name)

	eBPFEventReadChan := eventcenter.DefInstance.Subscribe(pvp.name, eventcenter.EBPF_EVENT_PROCESS_EXIT)

loop:
	for {
		select {
		case <-pvp.stopChan:
			glog.Warningf("eBPFProgram:'%s' handling eBPF Data goroutine receive stop notify", pvp.name)
			break loop
		case eBPFEvtInfo, ok := <-eBPFEventReadChan.C:
			if ok {
				switch eBPFEvtInfo.EvtType {
				case eventcenter.EBPF_EVENT_PROCESS_EXIT:
					processExitEvt := eBPFEvtInfo.EvtData.(*eventcenter.EBPFEventProcessExit)
					if config.ProgramCommFilter(pvp.name, processExitEvt.Comm) {
						pvp.processVMMapGuard.Lock()
						glog.Infof("eBPFProgram:'%s', receive eBPF Event'%s'", pvp.name, litter.Sdump(eBPFEvtInfo))
						pvp.processVMMap.Remove(processExitEvt.Pid)
						pvp.processVMMapGuard.Unlock()
					}
				}
				eBPFEvtInfo = nil
			}
		case data, ok := <-pvp.processVMEvtDataChan.C:
			if ok {
				comm := bpfutils.CommToString(data.Comm[:])
				if config.ProgramCommFilter(pvp.name, comm) {

					pvp.processVMMapGuard.Lock()

					opStr, _ := strings.CutPrefix(data.EvtType.String(), "XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_")

					if pvmI, ok := pvp.processVMMap.Get(data.Pid); !ok {
						pvm := new(processVM)
						pvm.pid = data.Pid
						pvm.comm = comm
						pvm.mmapAddrSet = hashset.New()

						switch data.EvtType {
						case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV:
							fallthrough
						case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED:
							pvm.mmapSize = (int64)(data.Len)
							// 将地址加入set
							pvm.mmapAddrSet.Add(data.Addr)
						case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK:
							pvm.brkSize = (int64)(data.Len)
						}
						pvp.processVMMap.Put(pvm.pid, pvm)
						glog.Infof("eBPFProgram:'%s', count:%d, new comm:'%s', pid:%d, mmapSize:%d, brkSize:%d, '%s':%d bytes, addr:%x in VM",
							pvp.name, pvp.processVMMap.Size(), pvm.comm, pvm.pid, pvm.mmapSize, pvm.brkSize, opStr, data.Len, data.Addr)
					} else {
						if pvm, ok := pvmI.(*processVM); ok {
							if pvm.comm == comm {
								switch data.EvtType {
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV:
									fallthrough
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED:
									pvm.mmapAddrSet.Add(data.Addr)
									pvm.mmapSize += (int64)(data.Len)
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK:
									pvm.brkSize += (int64)(data.Len)
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK_SHRINK:
									pvm.brkSize -= (int64)(data.Len)
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MUNMAP:
									if ok := pvm.mmapAddrSet.Contains(data.Addr); ok {
										pvm.mmapAddrSet.Remove(data.Addr)
										pvm.mmapSize -= (int64)(data.Len)
										glog.Infof("eBPFProgram:'%s' comm:'%s', pid:%d munmap addr:%x, len:%d", pvp.name, pvm.comm, pvm.pid,
											data.Addr, data.Len)
									} else {
										glog.Warningf("eBPFProgram:'%s' comm:'%s', pid:%d munmap addr:%x, len:%d not in mmapAddrSet", pvp.name, pvm.comm, pvm.pid,
											data.Addr, data.Len)
										prevAddr := data.Addr - data.Len
										if ok := pvm.mmapAddrSet.Contains(prevAddr); ok {
											pvm.mmapSize -= (int64)(data.Len)
											glog.Warningf("eBPFProgram:'%s' pid:%d munmap addr offset forward by %d bytes, the prevAddr:%x in mmapAddrSet",
												pvp.name, pvm.pid, data.Len, prevAddr)
										}
									}
								}
								glog.Infof("eBPFProgram:'%s', count:%d, comm:'%s', pid:%d, mmapSize:%d, brkSize:%d, '%s':%d bytes, addr:%x in VM",
									pvp.name, pvp.processVMMap.Size(), pvm.comm, pvm.pid, pvm.mmapSize, pvm.brkSize, opStr, data.Len, data.Addr)
							} else {
								// 进程不同
								prevComm := pvm.comm
								pvp.processVMMap.Remove(data.Pid)
								pvm := new(processVM)
								pvm.pid = data.Pid
								pvm.comm = comm
								pvm.mmapAddrSet = hashset.New()
								switch data.EvtType {
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV:
									fallthrough
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED:
									pvm.mmapSize = (int64)(data.Len)
									pvm.mmapAddrSet.Add(data.Addr)
								case bpfmodule.XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK:
									pvm.brkSize = (int64)(data.Len)
								}
								pvp.processVMMap.Put(pvm.pid, pvm)
								glog.Infof("eBPFProgram:'%s', count:%d, change prev:'%s', comm:'%s', pid:%d, mmapSize:%d, brkSize:%d, '%s':%d bytes, addr:%x in VM",
									pvp.name, pvp.processVMMap.Size(), prevComm, pvm.comm, pvm.pid, pvm.mmapSize, pvm.brkSize, opStr, data.Len, data.Addr)
							}
						}
					}
					pvp.processVMMapGuard.Unlock()
				}
				data = nil
			}
		}
	}
}

func (pvp *processVMProgram) Update(ch chan<- prometheus.Metric) error {
	glog.Infof("eBPFProgram:'%s' update start...", pvp.name)
	defer glog.Infof("eBPFProgram:'%s' update done.", pvp.name)

	count := 0
	mapstructure.Decode(config.ProgramConfigByName(pvp.name).Args.Private, &__pvmPrivateArgs)

	pvp.processVMMapGuard.Lock()
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

	for i := 0; i < count && i < __pvmPrivateArgs.ObjectCount; i++ {
		pvm := processVMs[i]
		// glog.Infof("eBPFProgram:'%s' pid:%d, comm:'%s', mmapSize:%d, brkSize:%d",
		// 	pvp.name, pvm.pid, pvm.comm, pvm.mmapSize, pvm.brkSize)
		pidStr := strconv.FormatInt(int64(pvm.pid), 10)
		// 指标值是page数量
		ch <- prometheus.MustNewConstMetric(pvp.addressSpaceExpandDesc,
			prometheus.GaugeValue, float64(pvm.mmapSize>>pageShift),
			pidStr, pvm.comm, "mmap", __pvmEBpfArgs.FilterScopeType.String(), strconv.Itoa(__pvmEBpfArgs.FilterScopeValue))
		ch <- prometheus.MustNewConstMetric(pvp.addressSpaceExpandDesc,
			prometheus.GaugeValue, float64(pvm.brkSize>>pageShift),
			pidStr, pvm.comm, "brk", __pvmEBpfArgs.FilterScopeType.String(), strconv.Itoa(__pvmEBpfArgs.FilterScopeValue))
	}

	pvp.processVMMapGuard.Unlock()

	// 释放
	processVMs = nil
	return nil
}

func (pvp *processVMProgram) Stop() {
	if pvp.vmmEvtRD != nil {
		pvp.vmmEvtRD.Close()
	}

	pvp.finalizer()

	if pvp.objs != nil {
		pvp.objs.Close()
		pvp.objs = nil
	}

	pvp.processVMEvtDataChan.SafeClose()
	glog.Infof("eBPFProgram:'%s' stopped", pvp.name)
}

func (pvp *processVMProgram) Reload() error {
	return nil
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
