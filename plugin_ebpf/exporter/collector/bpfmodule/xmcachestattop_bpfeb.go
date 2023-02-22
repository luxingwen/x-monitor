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

type XMCacheStatTopCachestatValue struct {
	AddToPageCacheLru    uint64
	IpAddToPageCache     uint64
	MarkPageAccessed     uint64
	IpMarkPageAccessed   uint64
	AccountPageDirtied   uint64
	IpAccountPageDirtied uint64
	MarkBufferDirty      uint64
	IpMarkBufferDirty    uint64
	Uid                  uint32
	Comm                 [16]int8
	_                    [4]byte
}

// LoadXMCacheStatTop returns the embedded CollectionSpec for XMCacheStatTop.
func LoadXMCacheStatTop() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMCacheStatTopBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMCacheStatTop: %w", err)
	}

	return spec, err
}

// LoadXMCacheStatTopObjects loads XMCacheStatTop and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMCacheStatTopObjects
//	*XMCacheStatTopPrograms
//	*XMCacheStatTopMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMCacheStatTopObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMCacheStatTop()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMCacheStatTopSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCacheStatTopSpecs struct {
	XMCacheStatTopProgramSpecs
	XMCacheStatTopMapSpecs
}

// XMCacheStatTopSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCacheStatTopProgramSpecs struct {
	XmCstAccountPageDirtied *ebpf.ProgramSpec `ebpf:"xm_cst_account_page_dirtied"`
	XmCstAddToPageCacheLru  *ebpf.ProgramSpec `ebpf:"xm_cst_add_to_page_cache_lru"`
	XmCstMarkBufferDirty    *ebpf.ProgramSpec `ebpf:"xm_cst_mark_buffer_dirty"`
	XmCstMarkPageAccessed   *ebpf.ProgramSpec `ebpf:"xm_cst_mark_page_accessed"`
}

// XMCacheStatTopMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMCacheStatTopMapSpecs struct {
	XmCachestatTopMap *ebpf.MapSpec `ebpf:"xm_cachestat_top_map"`
}

// XMCacheStatTopObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMCacheStatTopObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCacheStatTopObjects struct {
	XMCacheStatTopPrograms
	XMCacheStatTopMaps
}

func (o *XMCacheStatTopObjects) Close() error {
	return _XMCacheStatTopClose(
		&o.XMCacheStatTopPrograms,
		&o.XMCacheStatTopMaps,
	)
}

// XMCacheStatTopMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMCacheStatTopObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCacheStatTopMaps struct {
	XmCachestatTopMap *ebpf.Map `ebpf:"xm_cachestat_top_map"`
}

func (m *XMCacheStatTopMaps) Close() error {
	return _XMCacheStatTopClose(
		m.XmCachestatTopMap,
	)
}

// XMCacheStatTopPrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMCacheStatTopObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMCacheStatTopPrograms struct {
	XmCstAccountPageDirtied *ebpf.Program `ebpf:"xm_cst_account_page_dirtied"`
	XmCstAddToPageCacheLru  *ebpf.Program `ebpf:"xm_cst_add_to_page_cache_lru"`
	XmCstMarkBufferDirty    *ebpf.Program `ebpf:"xm_cst_mark_buffer_dirty"`
	XmCstMarkPageAccessed   *ebpf.Program `ebpf:"xm_cst_mark_page_accessed"`
}

func (p *XMCacheStatTopPrograms) Close() error {
	return _XMCacheStatTopClose(
		p.XmCstAccountPageDirtied,
		p.XmCstAddToPageCacheLru,
		p.XmCstMarkBufferDirty,
		p.XmCstMarkPageAccessed,
	)
}

func _XMCacheStatTopClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmcachestattop_bpfeb.o
var _XMCacheStatTopBytes []byte
