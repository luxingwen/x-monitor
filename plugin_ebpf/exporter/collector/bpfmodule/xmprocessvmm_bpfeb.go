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

type XMProcessVMMXmVmmEvtData struct {
	Pid     int32
	Tgid    int32
	Comm    [16]int8
	EvtType XMProcessVMMXmVmmEvtType
	_       [4]byte
	Len     uint64
}

type XMProcessVMMXmVmmEvtType uint32

const (
	XMProcessVMMXmVmmEvtTypeXM_VMM_EVT_TYPE_NONE           XMProcessVMMXmVmmEvtType = 0
	XMProcessVMMXmVmmEvtTypeXM_VMM_EVT_TYPE_MMAP_ANON_PRIV XMProcessVMMXmVmmEvtType = 1
	XMProcessVMMXmVmmEvtTypeXM_VMM_EVT_TYPE_MMAP_SHARED    XMProcessVMMXmVmmEvtType = 2
	XMProcessVMMXmVmmEvtTypeXM_VMM_EVT_TYPE_MMAP_OTHER     XMProcessVMMXmVmmEvtType = 3
	XMProcessVMMXmVmmEvtTypeXM_VMM_EVT_TYPE_BRK            XMProcessVMMXmVmmEvtType = 4
)

// LoadXMProcessVMM returns the embedded CollectionSpec for XMProcessVMM.
func LoadXMProcessVMM() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMProcessVMMBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMProcessVMM: %w", err)
	}

	return spec, err
}

// LoadXMProcessVMMObjects loads XMProcessVMM and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMProcessVMMObjects
//	*XMProcessVMMPrograms
//	*XMProcessVMMMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMProcessVMMObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMProcessVMM()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMProcessVMMSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProcessVMMSpecs struct {
	XMProcessVMMProgramSpecs
	XMProcessVMMMapSpecs
}

// XMProcessVMMSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProcessVMMProgramSpecs struct {
	XmProcessVmDoBrkFlags *ebpf.ProgramSpec `ebpf:"xm_process_vm_do_brk_flags"`
	XmProcessVmDoMmap     *ebpf.ProgramSpec `ebpf:"xm_process_vm_do_mmap"`
}

// XMProcessVMMMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProcessVMMMapSpecs struct {
	XmVmmEventRingbufMap *ebpf.MapSpec `ebpf:"xm_vmm_event_ringbuf_map"`
}

// XMProcessVMMObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMProcessVMMObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProcessVMMObjects struct {
	XMProcessVMMPrograms
	XMProcessVMMMaps
}

func (o *XMProcessVMMObjects) Close() error {
	return _XMProcessVMMClose(
		&o.XMProcessVMMPrograms,
		&o.XMProcessVMMMaps,
	)
}

// XMProcessVMMMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMProcessVMMObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProcessVMMMaps struct {
	XmVmmEventRingbufMap *ebpf.Map `ebpf:"xm_vmm_event_ringbuf_map"`
}

func (m *XMProcessVMMMaps) Close() error {
	return _XMProcessVMMClose(
		m.XmVmmEventRingbufMap,
	)
}

// XMProcessVMMPrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMProcessVMMObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProcessVMMPrograms struct {
	XmProcessVmDoBrkFlags *ebpf.Program `ebpf:"xm_process_vm_do_brk_flags"`
	XmProcessVmDoMmap     *ebpf.Program `ebpf:"xm_process_vm_do_mmap"`
}

func (p *XMProcessVMMPrograms) Close() error {
	return _XMProcessVMMClose(
		p.XmProcessVmDoBrkFlags,
		p.XmProcessVmDoMmap,
	)
}

func _XMProcessVMMClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmprocessvmm_bpfeb.o
var _XMProcessVMMBytes []byte
