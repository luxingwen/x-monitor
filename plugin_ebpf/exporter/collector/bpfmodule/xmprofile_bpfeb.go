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

type XMProfileStackTraceType [127]uint64

type XMProfileXmPidMaps struct {
	ModuleCount uint32
	_           [4]byte
	Modules     [32]XMProfileXmProcMapsModule
}

type XMProfileXmProcMapsModule struct {
	StartAddr   uint64
	EndAddr     uint64
	BuildIdHash uint64
	Type        uint32
	Path        [128]int8
	_           [4]byte
}

type XMProfileXmProfileDwRule struct {
	Rule   uint32
	_      [4]byte
	Offset int64
	Reg    uint64
}

type XMProfileXmProfileFdeRow struct {
	Loc uint64
	Cfa XMProfileXmProfileDwRule
	Rbp XMProfileXmProfileDwRule
	Ra  XMProfileXmProfileDwRule
}

type XMProfileXmProfileFdeTable struct {
	Start    uint64
	End      uint64
	Rows     [127]XMProfileXmProfileFdeRow
	RowCount uint32
	_        [4]byte
}

type XMProfileXmProfileModuleFdeTables struct {
	RefCount      uint32
	FdeTableCount uint32
	FdeTables     [2048]XMProfileXmProfileFdeTable
}

type XMProfileXmProfileSample struct {
	Pid           int32
	KernelStackId int32
	UserStackId   int32
	Comm          [16]int8
}

type XMProfileXmProfileSampleData struct {
	Count uint32
	PidNs uint32
	Pgid  int32
}

type XMProfileXmProgFilterArgs struct {
	ScopeType       XMProfileXmProgFilterTargetScopeType
	_               [4]byte
	FilterCondValue uint64
}

type XMProfileXmProgFilterTargetScopeType uint32

const (
	XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_NONE XMProfileXmProgFilterTargetScopeType = 0
	XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_OS   XMProfileXmProgFilterTargetScopeType = 1
	XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_NS   XMProfileXmProgFilterTargetScopeType = 2
	XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_CG   XMProfileXmProgFilterTargetScopeType = 3
	XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_PID  XMProfileXmProgFilterTargetScopeType = 4
	XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_PGID XMProfileXmProgFilterTargetScopeType = 5
	XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_MAX  XMProfileXmProgFilterTargetScopeType = 6
)

// LoadXMProfile returns the embedded CollectionSpec for XMProfile.
func LoadXMProfile() (*ebpf.CollectionSpec, error) {
	reader := bytes.NewReader(_XMProfileBytes)
	spec, err := ebpf.LoadCollectionSpecFromReader(reader)
	if err != nil {
		return nil, fmt.Errorf("can't load XMProfile: %w", err)
	}

	return spec, err
}

// LoadXMProfileObjects loads XMProfile and converts it into a struct.
//
// The following types are suitable as obj argument:
//
//	*XMProfileObjects
//	*XMProfilePrograms
//	*XMProfileMaps
//
// See ebpf.CollectionSpec.LoadAndAssign documentation for details.
func LoadXMProfileObjects(obj interface{}, opts *ebpf.CollectionOptions) error {
	spec, err := LoadXMProfile()
	if err != nil {
		return err
	}

	return spec.LoadAndAssign(obj, opts)
}

// XMProfileSpecs contains maps and programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProfileSpecs struct {
	XMProfileProgramSpecs
	XMProfileMapSpecs
}

// XMProfileSpecs contains programs before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProfileProgramSpecs struct {
	XmDoPerfEvent *ebpf.ProgramSpec `ebpf:"xm_do_perf_event"`
}

// XMProfileMapSpecs contains maps before they are loaded into the kernel.
//
// It can be passed ebpf.CollectionSpec.Assign.
type XMProfileMapSpecs struct {
	XmProfileArgMap            *ebpf.MapSpec `ebpf:"xm_profile_arg_map"`
	XmProfileModuleFdetableMap *ebpf.MapSpec `ebpf:"xm_profile_module_fdetable_map"`
	XmProfilePidModulesMap     *ebpf.MapSpec `ebpf:"xm_profile_pid_modules_map"`
	XmProfileSampleCountMap    *ebpf.MapSpec `ebpf:"xm_profile_sample_count_map"`
	XmProfileStackMap          *ebpf.MapSpec `ebpf:"xm_profile_stack_map"`
}

// XMProfileObjects contains all objects after they have been loaded into the kernel.
//
// It can be passed to LoadXMProfileObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProfileObjects struct {
	XMProfilePrograms
	XMProfileMaps
}

func (o *XMProfileObjects) Close() error {
	return _XMProfileClose(
		&o.XMProfilePrograms,
		&o.XMProfileMaps,
	)
}

// XMProfileMaps contains all maps after they have been loaded into the kernel.
//
// It can be passed to LoadXMProfileObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProfileMaps struct {
	XmProfileArgMap            *ebpf.Map `ebpf:"xm_profile_arg_map"`
	XmProfileModuleFdetableMap *ebpf.Map `ebpf:"xm_profile_module_fdetable_map"`
	XmProfilePidModulesMap     *ebpf.Map `ebpf:"xm_profile_pid_modules_map"`
	XmProfileSampleCountMap    *ebpf.Map `ebpf:"xm_profile_sample_count_map"`
	XmProfileStackMap          *ebpf.Map `ebpf:"xm_profile_stack_map"`
}

func (m *XMProfileMaps) Close() error {
	return _XMProfileClose(
		m.XmProfileArgMap,
		m.XmProfileModuleFdetableMap,
		m.XmProfilePidModulesMap,
		m.XmProfileSampleCountMap,
		m.XmProfileStackMap,
	)
}

// XMProfilePrograms contains all programs after they have been loaded into the kernel.
//
// It can be passed to LoadXMProfileObjects or ebpf.CollectionSpec.LoadAndAssign.
type XMProfilePrograms struct {
	XmDoPerfEvent *ebpf.Program `ebpf:"xm_do_perf_event"`
}

func (p *XMProfilePrograms) Close() error {
	return _XMProfileClose(
		p.XmDoPerfEvent,
	)
}

func _XMProfileClose(closers ...io.Closer) error {
	for _, closer := range closers {
		if err := closer.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Do not access this directly.
//
//go:embed xmprofile_bpfeb.o
var _XMProfileBytes []byte
