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

type XMRunQLatXmRunqlatHist struct{ Slots [26]uint32 }

// LoadXMRunQLat returns the embedded CollectionSpec for XMRunQLat.
func LoadXMRunQLat() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMRunQLatBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMRunQLat: %w", err)
	}

	return spec, err
}

// LoadXMRunQLatObjects loads XMRunQLat and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMRunQLatObjects
//	*XMRunQLatPrograms
//	*XMRunQLatMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMRunQLatObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMRunQLat()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMRunQLatSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMRunQLatSpecs struct {
	XMRunQLatProgramSpecs
	XMRunQLatMapSpecs
}

// XMRunQLatSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMRunQLatProgramSpecs struct {
	XmBtpSchedSwitch    *ebpf.ProgramSpec `ebpf:"xm_btp_sched_switch"`
	XmBtpSchedWakeup    *ebpf.ProgramSpec `ebpf:"xm_btp_sched_wakeup"`
	XmBtpSchedWakeupNew *ebpf.ProgramSpec `ebpf:"xm_btp_sched_wakeup_new"`
}

// XMRunQLatMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMRunQLatMapSpecs struct {
	XmRunqlatCgroupMap       *ebpf.MapSpec `ebpf:"xm_runqlat_cgroup_map"`
	XmRunqlatHistsMap        *ebpf.MapSpec `ebpf:"xm_runqlat_hists_map"`
	XmRunqlatThreadWakeupMap *ebpf.MapSpec `ebpf:"xm_runqlat_thread_wakeup_map"`
}

// XMRunQLatObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMRunQLatObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMRunQLatObjects struct {
	XMRunQLatPrograms
	XMRunQLatMaps
}

func (o *XMRunQLatObjects) Close() error {
	return _XMRunQLatClose(
		&o.XMRunQLatPrograms,
		&o.XMRunQLatMaps,
	)
}

// XMRunQLatMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMRunQLatObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMRunQLatMaps struct {
	XmRunqlatCgroupMap       *ebpf.Map `ebpf:"xm_runqlat_cgroup_map"`
	XmRunqlatHistsMap        *ebpf.Map `ebpf:"xm_runqlat_hists_map"`
	XmRunqlatThreadWakeupMap *ebpf.Map `ebpf:"xm_runqlat_thread_wakeup_map"`
}

func (m *XMRunQLatMaps) Close() error {
	return _XMRunQLatClose(
		m.XmRunqlatCgroupMap,
		m.XmRunqlatHistsMap,
		m.XmRunqlatThreadWakeupMap,
	)
}

// XMRunQLatPrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMRunQLatObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMRunQLatPrograms struct {
	XmBtpSchedSwitch    *ebpf.Program `ebpf:"xm_btp_sched_switch"`
	XmBtpSchedWakeup    *ebpf.Program `ebpf:"xm_btp_sched_wakeup"`
	XmBtpSchedWakeupNew *ebpf.Program `ebpf:"xm_btp_sched_wakeup_new"`
}

func (p *XMRunQLatPrograms) Close() error {
	return _XMRunQLatClose(
		p.XmBtpSchedSwitch,
		p.XmBtpSchedWakeup,
		p.XmBtpSchedWakeupNew,
	)
}

func _XMRunQLatClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmrunqlat_bpfel.o
var _XMRunQLatBytes []byte
