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

type XMNfsXmBlkNum struct {
	Major int32
	Minor int32
}

type XMNfsXmNfsOplatStat struct{ Slots [96]uint32 }

// LoadXMNfs returns the embedded CollectionSpec for XMNfs.
func LoadXMNfs() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMNfsBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMNfs: %w", err)
	}

	return spec, err
}

// LoadXMNfsObjects loads XMNfs and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMNfsObjects
//	*XMNfsPrograms
//	*XMNfsMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMNfsObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMNfs()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMNfsSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMNfsSpecs struct {
	XMNfsProgramSpecs
	XMNfsMapSpecs
}

// XMNfsSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMNfsProgramSpecs struct {
	XmKpNfsFileOpen   *ebpf.ProgramSpec `ebpf:"xm_kp_nfs_file_open"`
	XmKpNfsFileRead   *ebpf.ProgramSpec `ebpf:"xm_kp_nfs_file_read"`
	XmKpNfsFileWrite  *ebpf.ProgramSpec `ebpf:"xm_kp_nfs_file_write"`
	XmKpNfsGetattr    *ebpf.ProgramSpec `ebpf:"xm_kp_nfs_getattr"`
	XmKrpNfsFileOpen  *ebpf.ProgramSpec `ebpf:"xm_krp_nfs_file_open"`
	XmKrpNfsFileRead  *ebpf.ProgramSpec `ebpf:"xm_krp_nfs_file_read"`
	XmKrpNfsFileWrite *ebpf.ProgramSpec `ebpf:"xm_krp_nfs_file_write"`
	XmKrpNfsGetattr   *ebpf.ProgramSpec `ebpf:"xm_krp_nfs_getattr"`
}

// XMNfsMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMNfsMapSpecs struct {
	XmNfsOpTracingMap *ebpf.MapSpec `ebpf:"xm_nfs_op_tracing_map"`
	XmNfsOplatHistMap *ebpf.MapSpec `ebpf:"xm_nfs_oplat_hist_map"`
}

// XMNfsObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMNfsObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMNfsObjects struct {
	XMNfsPrograms
	XMNfsMaps
}

func (o *XMNfsObjects) Close() error {
	return _XMNfsClose(
		&o.XMNfsPrograms,
		&o.XMNfsMaps,
	)
}

// XMNfsMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMNfsObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMNfsMaps struct {
	XmNfsOpTracingMap *ebpf.Map `ebpf:"xm_nfs_op_tracing_map"`
	XmNfsOplatHistMap *ebpf.Map `ebpf:"xm_nfs_oplat_hist_map"`
}

func (m *XMNfsMaps) Close() error {
	return _XMNfsClose(
		m.XmNfsOpTracingMap,
		m.XmNfsOplatHistMap,
	)
}

// XMNfsPrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMNfsObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMNfsPrograms struct {
	XmKpNfsFileOpen   *ebpf.Program `ebpf:"xm_kp_nfs_file_open"`
	XmKpNfsFileRead   *ebpf.Program `ebpf:"xm_kp_nfs_file_read"`
	XmKpNfsFileWrite  *ebpf.Program `ebpf:"xm_kp_nfs_file_write"`
	XmKpNfsGetattr    *ebpf.Program `ebpf:"xm_kp_nfs_getattr"`
	XmKrpNfsFileOpen  *ebpf.Program `ebpf:"xm_krp_nfs_file_open"`
	XmKrpNfsFileRead  *ebpf.Program `ebpf:"xm_krp_nfs_file_read"`
	XmKrpNfsFileWrite *ebpf.Program `ebpf:"xm_krp_nfs_file_write"`
	XmKrpNfsGetattr   *ebpf.Program `ebpf:"xm_krp_nfs_getattr"`
}

func (p *XMNfsPrograms) Close() error {
	return _XMNfsClose(
		p.XmKpNfsFileOpen,
		p.XmKpNfsFileRead,
		p.XmKpNfsFileWrite,
		p.XmKpNfsGetattr,
		p.XmKrpNfsFileOpen,
		p.XmKrpNfsFileRead,
		p.XmKrpNfsFileWrite,
		p.XmKrpNfsGetattr,
	)
}

func _XMNfsClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmnfs_bpfeb.o
var _XMNfsBytes []byte
