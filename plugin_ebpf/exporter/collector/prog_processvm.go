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
	FilterScopeType  config.XMInternalResourceType `mapstructure:"filter_scope_type"`  // 过滤的资源类型
	FilterScopeValue int                           `mapstructure:"filter_scope_value"` // 过滤资源的值
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
	roData         *processVMMProgRodata
	excludeFilter  *eBPFProgramExclude
	gatherInterval time.Duration
	// ebpf
	vmmEvtRD *ringbuf.Reader
	objs     *bpfmodule.XMProcessVMObjects
	// manager
	processVMMap      *treemap.Map
	processVMMapGuard sync.Mutex
	// channel
	processVMDataEvtChan *bpfmodule.ProcessVMEvtDataChannel
}

func init() {
	registerEBPFProgram(processVMMProgName, newProcessVMMProgram)
}

func loadToRunProcessVMMProg(name string, prog *processVMProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMProcessVMObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMProcessVM, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_scope_type":  int32(prog.roData.FilterScopeType),
			"__filter_scope_value": int64(prog.roData.FilterScopeValue),
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
		roData:               new(processVMMProgRodata),
		excludeFilter:        new(eBPFProgramExclude),
		gatherInterval:       config.ProgramConfig(name).GatherInterval * time.Second,
		processVMMap:         treemap.NewWith(utils.Int32Comparator),
		processVMDataEvtChan: bpfmodule.NewProcessVMEvtDataChannel(defaultProcessVMEvtChanSize),
	}

	mapstructure.Decode(config.ProgramConfig(name).ProgRodata, prog.roData)
	mapstructure.Decode(config.ProgramConfig(name).Exclude, prog.excludeFilter)
	glog.Infof("eBPFProgram:'%s' roData:%s, exclude:%s", name, litter.Sdump(prog.roData), litter.Sdump(prog.excludeFilter))

	if err := loadToRunProcessVMMProg(name, prog); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' runProcessVMMProgram failed.", name)
		glog.Error(err)
		return nil, err
	}

	prog.wg.Go(prog.tracingProcessVMMEvent)
	prog.wg.Go(prog.processing)

	return prog, nil
}

func (pvp *processVMProgram) tracingProcessVMMEvent() {
	glog.Infof("eBPFProgram:'%s' start reading eEBP Prog data...", pvp.name)

loop:
	for {
		record, err := pvp.vmmEvtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningf("eBPFProgram:'%s' reading eEBP Prog data receive stop notify", pvp.name)
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

func (pvp *processVMProgram) processing() {
	glog.Infof("eBPFProgram:'%s' start processing processVM event data...", pvp.name)

	pvp.gatherTimer.Reset(pvp.gatherInterval)

loop:
	for {
		select {
		case <-pvp.stopChan:
			glog.Warningf("eBPFProgram:'%s' processing processVM event receive stop notify", pvp.name)
			break loop
		case data, ok := <-pvp.processVMDataEvtChan.C:
			if ok {
				comm := internal.CommToString(data.Comm[:])
				if !pvp.excludeFilter.IsExcludeComm(comm) {

					opStr, _ := strings.CutPrefix(data.EvtType.String(), "XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_")
					pvp.processVMMapGuard.Lock()

					if pvmI, ok := pvp.processVMMap.Get(data.Pid); !ok {
						pvm := new(processVM)
						pvm.pid = data.Pid
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
								pvp.processVMMap.Remove(data.Pid)
								pvm := new(processVM)
								pvm.pid = data.Pid
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

					pvp.processVMMapGuard.Unlock()
				}
			}
		case <-pvp.gatherTimer.Chan():
			pvp.processVMMapGuard.Lock()
			pvp.processVMMap.Clear()
			glog.Infof("eBPFProgram:'%s' clean up all processVMs!", pvp.name)
			pvp.processVMMapGuard.Unlock()
			pvp.gatherTimer.Reset(pvp.gatherInterval)
		}
	}
}

func (pvp *processVMProgram) Update(ch chan<- prometheus.Metric) error {
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
