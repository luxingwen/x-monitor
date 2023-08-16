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

// LoadXMCacheStat returns the embedded CollectionSpec for XMCacheStat.
func LoadXMCacheStat() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMCacheStatBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMCacheStat: %w", err)
	}

	return spec, err
}

// LoadXMCacheStatObjects loads XMCacheStat and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMCacheStatObjects
//	*XMCacheStatPrograms
//	*XMCacheStatMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMCacheStatObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMCacheStat()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMCacheStatSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCacheStatSpecs struct {
	XMCacheStatProgramSpecs
	XMCacheStatMapSpecs
}

// XMCacheStatSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCacheStatProgramSpecs struct {
	XmKpCsApd   *ebpf.ProgramSpec `ebpf:"xm_kp_cs_apd"`
	XmKpCsAtpcl *ebpf.ProgramSpec `ebpf:"xm_kp_cs_atpcl"`
	XmKpCsFad   *ebpf.ProgramSpec `ebpf:"xm_kp_cs_fad"`
	XmKpCsMbd   *ebpf.ProgramSpec `ebpf:"xm_kp_cs_mbd"`
	XmKpCsMpa   *ebpf.ProgramSpec `ebpf:"xm_kp_cs_mpa"`
}

// XMCacheStatMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCacheStatMapSpecs struct {
	XmPageCacheOpsCount *ebpf.MapSpec `ebpf:"xm_page_cache_ops_count"`
}

// XMCacheStatObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMCacheStatObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCacheStatObjects struct {
	XMCacheStatPrograms
	XMCacheStatMaps
}

func (o *XMCacheStatObjects) Close() error {
	return _XMCacheStatClose(
		&o.XMCacheStatPrograms,
		&o.XMCacheStatMaps,
	)
}

// XMCacheStatMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMCacheStatObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCacheStatMaps struct {
	XmPageCacheOpsCount *ebpf.Map `ebpf:"xm_page_cache_ops_count"`
}

func (m *XMCacheStatMaps) Close() error {
	return _XMCacheStatClose(
		m.XmPageCacheOpsCount,
	)
}

// XMCacheStatPrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMCacheStatObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCacheStatPrograms struct {
	XmKpCsApd   *ebpf.Program `ebpf:"xm_kp_cs_apd"`
	XmKpCsAtpcl *ebpf.Program `ebpf:"xm_kp_cs_atpcl"`
	XmKpCsFad   *ebpf.Program `ebpf:"xm_kp_cs_fad"`
	XmKpCsMbd   *ebpf.Program `ebpf:"xm_kp_cs_mbd"`
	XmKpCsMpa   *ebpf.Program `ebpf:"xm_kp_cs_mpa"`
}

func (p *XMCacheStatPrograms) Close() error {
	return _XMCacheStatClose(
		p.XmKpCsApd,
		p.XmKpCsAtpcl,
		p.XmKpCsFad,
		p.XmKpCsMbd,
		p.XmKpCsMpa,
	)
}

func _XMCacheStatClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmcachestat_bpfeb.o
var _XMCacheStatBytes []byte