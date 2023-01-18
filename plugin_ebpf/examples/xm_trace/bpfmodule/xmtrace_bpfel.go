// Code generated by bpf2go; DO NOT EDIT.
//go:build 386 || amd64 || amd64p32 || arm || arm64 || mips64le || mips64p32le || mipsle || ppc64le || riscv64
// +build 386 amd64 amd64p32 arm arm64 mips64le mips64p32le mipsle ppc64le riscv64

package bpfmodule

import (
	"bytes"
	_ "embed"
	"fmt"
	"io"

	"github.com/cilium/ebpf"
)

type XMTraceSyscallEvent struct {
	Pid           int32
	Tid           uint32
	SyscallNr     int64
	SyscallRet    int64
	CallStartNs   uint64
	CallDelayNs   uint64
	KernelStackId uint32
	UserStackId   uint32
}

// LoadXMTrace returns the embedded CollectionSpec for XMTrace.
func LoadXMTrace() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMTraceBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMTrace: %w", err)
	}

	return spec, err
}

// LoadXMTraceObjects loads XMTrace and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMTraceObjects
//	*XMTracePrograms
//	*XMTraceMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMTraceObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMTrace()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMTraceSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMTraceSpecs struct {
	XMTraceProgramSpecs
	XMTraceMapSpecs
}

// XMTraceSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMTraceProgramSpecs struct {
	XmTraceKpSysOpenat     *ebpf.ProgramSpec `ebpf:"xm_trace_kp__sys_openat"`
	XmTraceKpSysReadlinkat *ebpf.ProgramSpec `ebpf:"xm_trace_kp__sys_readlinkat"`
	XmTraceRawTpSysEnter   *ebpf.ProgramSpec `ebpf:"xm_trace_raw_tp__sys_enter"`
	XmTraceRawTpSysExit    *ebpf.ProgramSpec `ebpf:"xm_trace_raw_tp__sys_exit"`
}

// XMTraceMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMTraceMapSpecs struct {
	XmFilterSyscallsMap *ebpf.MapSpec `ebpf:"xm_filter_syscalls_map"`
	XmSyscallsEventMap  *ebpf.MapSpec `ebpf:"xm_syscalls_event_map"`
	XmSyscallsRecordMap *ebpf.MapSpec `ebpf:"xm_syscalls_record_map"`
	XmSyscallsStackMap  *ebpf.MapSpec `ebpf:"xm_syscalls_stack_map"`
}

// XMTraceObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMTraceObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMTraceObjects struct {
	XMTracePrograms
	XMTraceMaps
}

func (o *XMTraceObjects) Close() error {
	return _XMTraceClose(
		&o.XMTracePrograms,
		&o.XMTraceMaps,
	)
}

// XMTraceMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMTraceObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMTraceMaps struct {
	XmFilterSyscallsMap *ebpf.Map `ebpf:"xm_filter_syscalls_map"`
	XmSyscallsEventMap  *ebpf.Map `ebpf:"xm_syscalls_event_map"`
	XmSyscallsRecordMap *ebpf.Map `ebpf:"xm_syscalls_record_map"`
	XmSyscallsStackMap  *ebpf.Map `ebpf:"xm_syscalls_stack_map"`
}

func (m *XMTraceMaps) Close() error {
	return _XMTraceClose(
		m.XmFilterSyscallsMap,
		m.XmSyscallsEventMap,
		m.XmSyscallsRecordMap,
		m.XmSyscallsStackMap,
	)
}

// XMTracePrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMTraceObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMTracePrograms struct {
	XmTraceKpSysOpenat     *ebpf.Program `ebpf:"xm_trace_kp__sys_openat"`
	XmTraceKpSysReadlinkat *ebpf.Program `ebpf:"xm_trace_kp__sys_readlinkat"`
	XmTraceRawTpSysEnter   *ebpf.Program `ebpf:"xm_trace_raw_tp__sys_enter"`
	XmTraceRawTpSysExit    *ebpf.Program `ebpf:"xm_trace_raw_tp__sys_exit"`
}

func (p *XMTracePrograms) Close() error {
	return _XMTraceClose(
		p.XmTraceKpSysOpenat,
		p.XmTraceKpSysReadlinkat,
		p.XmTraceRawTpSysEnter,
		p.XmTraceRawTpSysExit,
	)
}

func _XMTraceClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmtrace_bpfel.o
var _XMTraceBytes []byte
