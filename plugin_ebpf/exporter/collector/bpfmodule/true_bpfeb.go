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

// loadTrue returns the embedded CollectionSpec for true.
func loadTrue() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_TrueBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load true: %w", err)
	}

	return spec, err
}

// loadTrueObjects loads true and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*trueObjects
//	*truePrograms
//	*trueMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func loadTrueObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := loadTrue()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// trueSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type trueSpecs struct {
	trueProgramSpecs
	trueMapSpecs
}

// trueSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type trueProgramSpecs struct {
	XmKpCsApd   *ebpf.ProgramSpec `ebpf:"xm_kp_cs_apd"`
	XmKpCsAtpcl *ebpf.ProgramSpec `ebpf:"xm_kp_cs_atpcl"`
	XmKpCsMbd   *ebpf.ProgramSpec `ebpf:"xm_kp_cs_mbd"`
	XmKpCsMpa   *ebpf.ProgramSpec `ebpf:"xm_kp_cs_mpa"`
}

// trueMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type trueMapSpecs struct {
}

// trueObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to loadTrueObjects or ebpf.CollectionSpec.LoadAndAssign.
type trueObjects struct {
	truePrograms
	trueMaps
}

func (o *trueObjects) Close() error {
	return _TrueClose(
		&o.truePrograms,
		&o.trueMaps,
	)
}

// trueMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to loadTrueObjects or ebpf.CollectionSpec.LoadAndAssign.
type trueMaps struct {
}

func (m *trueMaps) Close() error {
	return _TrueClose()
}

// truePrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to loadTrueObjects or ebpf.CollectionSpec.LoadAndAssign.
type truePrograms struct {
	XmKpCsApd   *ebpf.Program `ebpf:"xm_kp_cs_apd"`
	XmKpCsAtpcl *ebpf.Program `ebpf:"xm_kp_cs_atpcl"`
	XmKpCsMbd   *ebpf.Program `ebpf:"xm_kp_cs_mbd"`
	XmKpCsMpa   *ebpf.Program `ebpf:"xm_kp_cs_mpa"`
}

func (p *truePrograms) Close() error {
	return _TrueClose(
		p.XmKpCsApd,
		p.XmKpCsAtpcl,
		p.XmKpCsMbd,
		p.XmKpCsMpa,
	)
}

func _TrueClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed true_bpfeb.o
var _TrueBytes []byte
