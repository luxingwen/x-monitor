// Code generated by bpf2go; DO NOT EDIT.
//go:build 386 || amd64 || amd64p32 || arm || arm64 || loong64 || mips64le || mips64p32le || mipsle || ppc64le || riscv64

package bpfmodule

import (
	"bytes"
	_ "embed"
	"fmt"
	"io"

	"github.com/cilium/ebpf"
)

type XMProcessVMVmOpInfo struct {
	Type XMProcessVMXmProcessvmEvtType
	_    [4]byte
	Addr uint64
	Len  uint64
}

type XMProcessVMXmProcessvmEvtData struct {
	Tid     int32
	Pid     int32
	Comm    [16]int8
	EvtType XMProcessVMXmProcessvmEvtType
	_       [4]byte
	Addr    uint64
	Len     uint64
}

type XMProcessVMXmProcessvmEvtType uint32

const (
	XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_NONE           XMProcessVMXmProcessvmEvtType = 0
	XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV XMProcessVMXmProcessvmEvtType = 1
	XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED    XMProcessVMXmProcessvmEvtType = 2
	XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK            XMProcessVMXmProcessvmEvtType = 3
	XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK_SHRINK     XMProcessVMXmProcessvmEvtType = 4
	XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MUNMAP         XMProcessVMXmProcessvmEvtType = 5
	XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MAX            XMProcessVMXmProcessvmEvtType = 6
)

// LoadXMProcessVM returns the embedded CollectionSpec for XMProcessVM.
func LoadXMProcessVM() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMProcessVMBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMProcessVM: %w", err)
	}

	return spec, err
}

// LoadXMProcessVMObjects loads XMProcessVM and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMProcessVMObjects
//	*XMProcessVMPrograms
//	*XMProcessVMMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMProcessVMObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMProcessVM()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMProcessVMSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProcessVMSpecs struct {
	XMProcessVMProgramSpecs
	XMProcessVMMapSpecs
}

// XMProcessVMSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProcessVMProgramSpecs struct {
	KprobeXmDoMunmap        *ebpf.ProgramSpec `ebpf:"kprobe__xm___do_munmap"`
	KprobeXmDoBrkFlags      *ebpf.ProgramSpec `ebpf:"kprobe__xm_do_brk_flags"`
	KprobeXmDoMmap          *ebpf.ProgramSpec `ebpf:"kprobe__xm_do_mmap"`
	KprobeXmMmapRegion      *ebpf.ProgramSpec `ebpf:"kprobe__xm_mmap_region"`
	KretprobeXmDoMunmap     *ebpf.ProgramSpec `ebpf:"kretprobe__xm___do_munmap"`
	KretprobeXmDoBrkFlags   *ebpf.ProgramSpec `ebpf:"kretprobe__xm_do_brk_flags"`
	KretprobeXmDoMmap       *ebpf.ProgramSpec `ebpf:"kretprobe__xm_do_mmap"`
	TracepointXmSysEnterBrk *ebpf.ProgramSpec `ebpf:"tracepoint_xm_sys_enter_brk"`
}

// XMProcessVMMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProcessVMMapSpecs struct {
	HashmapXmVmOp              *ebpf.MapSpec `ebpf:"hashmap_xm_vm_op"`
	XmProcessvmEventRingbufMap *ebpf.MapSpec `ebpf:"xm_processvm_event_ringbuf_map"`
}

// XMProcessVMObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMProcessVMObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProcessVMObjects struct {
	XMProcessVMPrograms
	XMProcessVMMaps
}

func (o *XMProcessVMObjects) Close() error {
	return _XMProcessVMClose(
		&o.XMProcessVMPrograms,
		&o.XMProcessVMMaps,
	)
}

// XMProcessVMMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMProcessVMObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProcessVMMaps struct {
	HashmapXmVmOp              *ebpf.Map `ebpf:"hashmap_xm_vm_op"`
	XmProcessvmEventRingbufMap *ebpf.Map `ebpf:"xm_processvm_event_ringbuf_map"`
}

func (m *XMProcessVMMaps) Close() error {
	return _XMProcessVMClose(
		m.HashmapXmVmOp,
		m.XmProcessvmEventRingbufMap,
	)
}

// XMProcessVMPrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMProcessVMObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProcessVMPrograms struct {
	KprobeXmDoMunmap        *ebpf.Program `ebpf:"kprobe__xm___do_munmap"`
	KprobeXmDoBrkFlags      *ebpf.Program `ebpf:"kprobe__xm_do_brk_flags"`
	KprobeXmDoMmap          *ebpf.Program `ebpf:"kprobe__xm_do_mmap"`
	KprobeXmMmapRegion      *ebpf.Program `ebpf:"kprobe__xm_mmap_region"`
	KretprobeXmDoMunmap     *ebpf.Program `ebpf:"kretprobe__xm___do_munmap"`
	KretprobeXmDoBrkFlags   *ebpf.Program `ebpf:"kretprobe__xm_do_brk_flags"`
	KretprobeXmDoMmap       *ebpf.Program `ebpf:"kretprobe__xm_do_mmap"`
	TracepointXmSysEnterBrk *ebpf.Program `ebpf:"tracepoint_xm_sys_enter_brk"`
}

func (p *XMProcessVMPrograms) Close() error {
	return _XMProcessVMClose(
		p.KprobeXmDoMunmap,
		p.KprobeXmDoBrkFlags,
		p.KprobeXmDoMmap,
		p.KprobeXmMmapRegion,
		p.KretprobeXmDoMunmap,
		p.KretprobeXmDoBrkFlags,
		p.KretprobeXmDoMmap,
		p.TracepointXmSysEnterBrk,
	)
}

func _XMProcessVMClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmprocessvm_bpfel.o
var _XMProcessVMBytes []byte