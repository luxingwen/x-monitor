// Code generated by bpf2go; DO NOT EDIT.
//go:build arm64be || armbe || mips || mips64 || mips64p32 || ppc64 || s390 || s390x || sparc || sparc64

package bpfmodule

import (
	"bytes"
	_ "embed"
	"fmt"
	"io"

	"github.com/cilium/ebpf"
)

type XMCpuScheduleXmCpuSchedEvtData struct {
	OffcpuDurationMillsecs uint64
	KernelStackId          uint32
	UserStackId            uint32
	EvtType                XMCpuScheduleXmCpuSchedEvtType
	Tid                    int32
	Pid                    int32
	Comm                   [16]int8
	_                      [4]byte
}

type XMCpuScheduleXmCpuSchedEvtType uint32

const (
	XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_NONE         XMCpuScheduleXmCpuSchedEvtType = 0
	XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_RUNQLAT      XMCpuScheduleXmCpuSchedEvtType = 1
	XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_OFFCPU       XMCpuScheduleXmCpuSchedEvtType = 2
	XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_HANG         XMCpuScheduleXmCpuSchedEvtType = 3
	XMCpuScheduleXmCpuSchedEvtTypeXM_CS_EVT_TYPE_PROCESS_EXIT XMCpuScheduleXmCpuSchedEvtType = 4
)

type XMCpuScheduleXmRunqlatHist struct{ Slots [20]uint32 }

// LoadXMCpuSchedule returns the embedded CollectionSpec for XMCpuSchedule.
func LoadXMCpuSchedule() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMCpuScheduleBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMCpuSchedule: %w", err)
	}

	return spec, err
}

// LoadXMCpuScheduleObjects loads XMCpuSchedule and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMCpuScheduleObjects
//	*XMCpuSchedulePrograms
//	*XMCpuScheduleMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMCpuScheduleObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMCpuSchedule()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMCpuScheduleSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCpuScheduleSpecs struct {
	XMCpuScheduleProgramSpecs
	XMCpuScheduleMapSpecs
}

// XMCpuScheduleSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCpuScheduleProgramSpecs struct {
	BtfTracepointXmSchedProcessExit *ebpf.ProgramSpec `ebpf:"btf_tracepoint__xm_sched_process_exit"`
	BtfTracepointXmSchedProcessHang *ebpf.ProgramSpec `ebpf:"btf_tracepoint__xm_sched_process_hang"`
	BtfTracepointXmSchedSwitch      *ebpf.ProgramSpec `ebpf:"btf_tracepoint__xm_sched_switch"`
	BtfTracepointXmSchedWakeup      *ebpf.ProgramSpec `ebpf:"btf_tracepoint__xm_sched_wakeup"`
	BtfTracepointXmSchedWakeupNew   *ebpf.ProgramSpec `ebpf:"btf_tracepoint__xm_sched_wakeup_new"`
}

// XMCpuScheduleMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCpuScheduleMapSpecs struct {
	XmCsEventRingbufMap *ebpf.MapSpec `ebpf:"xm_cs_event_ringbuf_map"`
	XmCsOffcpuStartMap  *ebpf.MapSpec `ebpf:"xm_cs_offcpu_start_map"`
	XmCsRunqlatHistsMap *ebpf.MapSpec `ebpf:"xm_cs_runqlat_hists_map"`
	XmCsRunqlatStartMap *ebpf.MapSpec `ebpf:"xm_cs_runqlat_start_map"`
	XmCsStackMap        *ebpf.MapSpec `ebpf:"xm_cs_stack_map"`
}

// XMCpuScheduleObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMCpuScheduleObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCpuScheduleObjects struct {
	XMCpuSchedulePrograms
	XMCpuScheduleMaps
}

func (o *XMCpuScheduleObjects) Close() error {
	return _XMCpuScheduleClose(
		&o.XMCpuSchedulePrograms,
		&o.XMCpuScheduleMaps,
	)
}

// XMCpuScheduleMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMCpuScheduleObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCpuScheduleMaps struct {
	XmCsEventRingbufMap *ebpf.Map `ebpf:"xm_cs_event_ringbuf_map"`
	XmCsOffcpuStartMap  *ebpf.Map `ebpf:"xm_cs_offcpu_start_map"`
	XmCsRunqlatHistsMap *ebpf.Map `ebpf:"xm_cs_runqlat_hists_map"`
	XmCsRunqlatStartMap *ebpf.Map `ebpf:"xm_cs_runqlat_start_map"`
	XmCsStackMap        *ebpf.Map `ebpf:"xm_cs_stack_map"`
}

func (m *XMCpuScheduleMaps) Close() error {
	return _XMCpuScheduleClose(
		m.XmCsEventRingbufMap,
		m.XmCsOffcpuStartMap,
		m.XmCsRunqlatHistsMap,
		m.XmCsRunqlatStartMap,
		m.XmCsStackMap,
	)
}

// XMCpuSchedulePrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMCpuScheduleObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCpuSchedulePrograms struct {
	BtfTracepointXmSchedProcessExit *ebpf.Program `ebpf:"btf_tracepoint__xm_sched_process_exit"`
	BtfTracepointXmSchedProcessHang *ebpf.Program `ebpf:"btf_tracepoint__xm_sched_process_hang"`
	BtfTracepointXmSchedSwitch      *ebpf.Program `ebpf:"btf_tracepoint__xm_sched_switch"`
	BtfTracepointXmSchedWakeup      *ebpf.Program `ebpf:"btf_tracepoint__xm_sched_wakeup"`
	BtfTracepointXmSchedWakeupNew   *ebpf.Program `ebpf:"btf_tracepoint__xm_sched_wakeup_new"`
}

func (p *XMCpuSchedulePrograms) Close() error {
	return _XMCpuScheduleClose(
		p.BtfTracepointXmSchedProcessExit,
		p.BtfTracepointXmSchedProcessHang,
		p.BtfTracepointXmSchedSwitch,
		p.BtfTracepointXmSchedWakeup,
		p.BtfTracepointXmSchedWakeupNew,
	)
}

func _XMCpuScheduleClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmcpuschedule_bpfeb.o
var _XMCpuScheduleBytes []byte
