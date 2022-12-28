// Code generated by bpf2go; DO NOT EDIT.
//go:build arm64be || armbe || mips || mips64 || mips64p32 || ppc64 || s390 || s390x || sparc || sparc64
// +build arm64be armbe mips mips64 mips64p32 ppc64 s390 s390x sparc sparc64

package bpfmodule

import (
	"bytes"
	_ "embed"
	"fmt"
	"io"

	"github.com/cilium/ebpf"
)

type TraceProcessSyscallsXmTraceSyscallDatarec struct {
	SyscallTotalCount  uint64
	SyscallFailedCount uint64
	SyscallTotalNs     uint64
	SyscallSlowestNs   uint64
	SyscallSlowestNr   uint64
	SyscallFailedNr    uint64
	SyscallErrno       int32
	_                  [4]byte
}

// LoadTraceProcessSyscalls returns the embedded CollectionSpec for TraceProcessSyscalls.
func LoadTraceProcessSyscalls() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_TraceProcessSyscallsBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load TraceProcessSyscalls: %w", err)
	}

	return spec, err
}

// LoadTraceProcessSyscallsObjects loads TraceProcessSyscalls and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*TraceProcessSyscallsObjects
//	*TraceProcessSyscallsPrograms
//	*TraceProcessSyscallsMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadTraceProcessSyscallsObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadTraceProcessSyscalls()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// TraceProcessSyscallsSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type TraceProcessSyscallsSpecs struct {
	TraceProcessSyscallsProgramSpecs
	TraceProcessSyscallsMapSpecs
}

// TraceProcessSyscallsSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type TraceProcessSyscallsProgramSpecs struct {
	XmBtfRtpSysEnter *ebpf.ProgramSpec `ebpf:"xm_btf_rtp__sys_enter"`
	XmBtfRtpSysExit  *ebpf.ProgramSpec `ebpf:"xm_btf_rtp__sys_exit"`
}

// TraceProcessSyscallsMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type TraceProcessSyscallsMapSpecs struct {
	XmFilterSyscallsMap       *ebpf.MapSpec `ebpf:"xm_filter_syscalls_map"`
	XmTraceSyscallsDatarecMap *ebpf.MapSpec `ebpf:"xm_trace_syscalls_datarec_map"`
}

// TraceProcessSyscallsObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadTraceProcessSyscallsObjects or ebpf.CollectionSpec.LoadAndAssign.
type TraceProcessSyscallsObjects struct {
	TraceProcessSyscallsPrograms
	TraceProcessSyscallsMaps
}

func (o *TraceProcessSyscallsObjects) Close() error {
	return _TraceProcessSyscallsClose(
		&o.TraceProcessSyscallsPrograms,
		&o.TraceProcessSyscallsMaps,
	)
}

// TraceProcessSyscallsMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadTraceProcessSyscallsObjects or ebpf.CollectionSpec.LoadAndAssign.
type TraceProcessSyscallsMaps struct {
	XmFilterSyscallsMap       *ebpf.Map `ebpf:"xm_filter_syscalls_map"`
	XmTraceSyscallsDatarecMap *ebpf.Map `ebpf:"xm_trace_syscalls_datarec_map"`
}

func (m *TraceProcessSyscallsMaps) Close() error {
	return _TraceProcessSyscallsClose(
		m.XmFilterSyscallsMap,
		m.XmTraceSyscallsDatarecMap,
	)
}

// TraceProcessSyscallsPrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadTraceProcessSyscallsObjects or ebpf.CollectionSpec.LoadAndAssign.
type TraceProcessSyscallsPrograms struct {
	XmBtfRtpSysEnter *ebpf.Program `ebpf:"xm_btf_rtp__sys_enter"`
	XmBtfRtpSysExit  *ebpf.Program `ebpf:"xm_btf_rtp__sys_exit"`
}

func (p *TraceProcessSyscallsPrograms) Close() error {
	return _TraceProcessSyscallsClose(
		p.XmBtfRtpSysEnter,
		p.XmBtfRtpSysExit,
	)
}

func _TraceProcessSyscallsClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed traceprocesssyscalls_bpfeb.o
var _TraceProcessSyscallsBytes []byte
