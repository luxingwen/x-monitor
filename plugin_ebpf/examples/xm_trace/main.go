/*
 * @Author: CALM.WU
 * @Date: 2022-12-28 10:42:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-01-09 15:27:17
 */

package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	goflag "flag"
	"reflect"
	"strings"
	"time"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/asm"
	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/cilium/ebpf/rlimit"
	"github.com/golang/glog"
	"xmonitor.calmwu/plugin_ebpf/examples/xm_trace/bpfmodule"

	calmutils "github.com/wubo0067/calmwu-go/utils"
)

const (
	_perf_max_stack_depth = 20
)

var _pid int

func init() {
	goflag.IntVar(&_pid, "pid", 0, "trace process syscalls pid")
}

func __fillKprobeStackInstructions(progSpec *ebpf.ProgramSpec) {
	progSpec.Instructions = asm.Instructions{
		asm.StoreMem(asm.RFP, -16, asm.R1, asm.DWord),
		asm.LoadMapValue(asm.R1, 0, 64).WithReference(".rodata"),
		// *(u64 *)(r10 - 48) = r1,	StXMemDW dst: rfp src: r1 off: -48 imm: 0
		asm.StoreMem(asm.R1, -48, asm.R1, asm.DWord),
		// r1 = *(u64 *)(r1 + 0),	LdXMemDW dst: r1 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R1, asm.R1, 0, asm.DWord),
		// callx r1, Call FnMapLookupElem
		asm.FnMapLookupPercpuElem.Call(),
		// 6:	r1 = *(u64 *)(r10 - 48), LdXMemDW dst: r1 src: rfp off: -48 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -48, asm.DWord),
		// 7:	r0 >>= 32, RShImm dst: r0 imm: 32
		asm.RSh.Imm(asm.R0, 32),
		// 8:	*(u32 *)(r10 - 20) = r0, StXMemW dst: rfp src: r0 off: -20 imm: 0
		asm.StoreMem(asm.RFP, -20, asm.R0, asm.Word),
		// r1 = *(u64 *)(r1 + 0), LdXMemDW dst: r1 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R1, asm.R1, 0, asm.DWord),
		// 10:	callx r1, Call FnMapLookupElem
		asm.FnMapLookupPercpuElem.Call(),
		// 11:	*(u32 *)(r10 - 24) = r0, StXMemW dst: rfp src: r0 off: -24 imm: 0
		asm.StoreMem(asm.RFP, -24, asm.R0, asm.Word),
		// 12:	r1 = 0 ll, LoadMapValue dst: r1, fd: 0 off: 0 <.rodata>
		asm.LoadMapValue(asm.R1, 0, 0).WithReference(".rodata"),
		// 14:	r1 = *(u32 *)(r1 + 0), LdXMemW dst: r1 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R1, asm.R1, 0, asm.Word),
		// 15:	if r1 == 0 goto +12 <LBB0_3>, JEqImm dst: r1 off: 12 imm: 0
		asm.JEq.Imm(asm.R1, 0, "LBB0_3"),
		// 16:	goto +0 <LBB0_1>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_1"),
		// 17:	r1 = 0 ll, LoadMapValue dst: r1, fd: 0 off: 0 <.rodata>
		asm.LoadMapValue(asm.R1, 0, 0).WithSymbol("LBB0_1").WithReference(".rodata"),
		// 19:	r1 = *(u32 *)(r1 + 0), LdXMemW dst: r1 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R1, asm.R1, 0, asm.Word),
		// if r1 == 0 goto +10 <LBB0_4>, JEqImm dst: r1 off: 10 imm: 0
		asm.JEq.Imm(asm.R1, 0, "LBB0_4"),
		// 21:	goto +0 <LBB0_2>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_2"),
		// 22:	r1 = *(u32 *)(r10 - 20), LdXMemW dst: r1 src: rfp off: -20 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -20, asm.Word).WithSymbol("LBB0_2"),
		// 23:  r2 = 0 ll, LoadMapValue dst: r2, fd: 0 off: 0 <.rodata>
		asm.LoadMapValue(asm.R2, 0, 0).WithReference(".rodata"),
		// 25:	r2 = *(u32 *)(r2 + 0), LdXMemW dst: r2 src: r2 off: 0 imm: 0
		asm.LoadMem(asm.R2, asm.R2, 0, asm.Word),
		// 26:	if r1 == r2 goto +4 <LBB0_4>, JEqReg dst: r1 off: 4 src: r2
		asm.JEq.Reg(asm.R1, asm.R2, "LBB0_4"),
		// 27:	goto +0 <LBB0_3>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_3"),
		// 28:	r1 = 0, MovImm dst: r1 imm: 0
		asm.Mov.Imm(asm.R1, 0).WithSymbol("LBB0_3"),
		// 29:	*(u32 *)(r10 - 4) = r1, StXMemW dst: rfp src: r1 off: -4 imm: 0
		asm.StoreMem(asm.RFP, -4, asm.R1, asm.Word),
		// 30:	goto +54 <LBB0_11>, JaImm dst: r0 off: 54 imm: 0
		asm.Ja.Label("LBB0_11"),
		// 31:	r1 = 0 ll, LoadMapValue dst: r1, fd: 0 off: 0 <.data>
		asm.LoadMapValue(asm.R1, 0, 0).WithSymbol("LBB0_4").WithReference(".rodata"),
		// 33:	r3 = *(u64 *)(r1 + 0), LdXMemDW dst: r3 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R3, asm.R1, 0, asm.DWord),
		// 34:	r1 = 0 ll, LoadMapPtr dst: r1 fd: 0 <xm_syscalls_record_map>
		asm.LoadMapPtr(asm.R1, 0).WithReference("xm_syscalls_record_map"),
		// 36:	r2 = r10, MovReg dst: r2 src: rfp
		asm.Mov.Reg(asm.R2, asm.RFP),
		// 37:	r2 += -24, AddImm dst: r2 imm: -24
		asm.Add.Imm(asm.R2, -24),
		// 38:	callx r3, Call FnMapDeleteElem
		asm.FnMapDeleteElem.Call(),
		// 39:	*(u64 *)(r10 - 32) = r0, StXMemDW dst: rfp src: r0 off: -32 imm: 0
		asm.StoreMem(asm.RFP, -32, asm.R0, asm.DWord),
		// 40:	r1 = *(u64 *)(r10 - 32), LdXMemDW dst: r1 src: rfp off: -32 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -32, asm.DWord),
		// 41:	if r1 == 0 goto +40 <LBB0_10>, JEqImm dst: r1 off: 40 imm: 0
		asm.JEq.Imm(asm.R1, 0, "LBB0_10"),
		// 42:	goto +0 <LBB0_5>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_5"),
		// 43:	r1 = 8 ll, LoadMapValue dst: r1, fd: 0 off: 8 <.data>
		asm.LoadMapValue(asm.R1, 0, 8).WithSymbol("LBB0_5").WithReference(".rodata"),
		// 45:	*(u64 *)(r10 - 56) = r1, StXMemDW dst: rfp src: r1 off: -56 imm: 0
		asm.StoreMem(asm.RFP, -56, asm.R1, asm.DWord),
		// 46:	r4 = *(u64 *)(r1 + 0), LdXMemDW dst: r4 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R4, asm.R1, 0, asm.DWord),
		// 47:	r1 = *(u64 *)(r10 - 16), LdXMemDW dst: r1 src: rfp off: -16 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -16, asm.DWord),
		// 48:	r2 = 0 ll, LoadMapPtr dst: r2 fd: 0 <xm_syscalls_stack_map>
		asm.LoadMapPtr(asm.R2, 0).WithReference("xm_syscalls_stack_map"),
		// 50:	*(u64 *)(r10 - 64) = r2, StXMemDW dst: rfp src: r2 off: -64 imm: 0
		asm.StoreMem(asm.RFP, -64, asm.R2, asm.DWord),
		// 51:	r3 = 512, MovImm dst: r3 imm: 512
		asm.Mov.Imm(asm.R3, 512),
		// 52:	callx r4, Call FnProbeRead
		asm.FnProbeRead.Call(),
		// 53:	r2 = *(u64 *)(r10 - 64), LdXMemDW dst: r2 src: rfp off: -64 imm: 0
		asm.LoadMem(asm.R2, asm.RFP, -64, asm.DWord),
		// 54:	r1 = *(u64 *)(r10 - 56), LdXMemDW dst: r1 src: rfp off: -56 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -56, asm.DWord),
		// 55:	*(u32 *)(r10 - 36) = r0, StXMemW dst: rfp src: r0 off: -36 imm: 0
		asm.StoreMem(asm.RFP, -36, asm.R0, asm.Word),
		// 56:	r4 = *(u64 *)(r1 + 0), LdXMemDW dst: r4 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R4, asm.R1, 0, asm.DWord),
		// 57:	r1 = *(u64 *)(r10 - 16), LdXMemDW dst: r1 src: rfp off: -16 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -16, asm.DWord),
		// 58:	r3 = 768, MovImm dst: r3 imm: 768
		asm.Mov.Imm(asm.R3, 768),
		// 59:	callx r4, Call FnProbeRead
		asm.FnProbeRead.Call(),
		// 60:	*(u32 *)(r10 - 40) = r0, StXMemW dst: rfp src: r0 off: -40 imm: 0
		asm.StoreMem(asm.RFP, -40, asm.R0, asm.Word),
		// 61:	r2 = *(u32 *)(r10 - 36), LdXMemW dst: r2 src: rfp off: -36 imm: 0
		asm.LoadMem(asm.R2, asm.RFP, -36, asm.Word),
		// 62:	r2 <<= 32, LShImm dst: r2 imm: 32
		asm.LSh.Imm(asm.R2, 32),
		// 63:	r2 s>>= 32, ArShImm dst: r2 imm: 32
		asm.ArSh.Imm(asm.R2, 32),
		// 64:	r1 = 1, MovImm dst: r1 imm: 1
		asm.Mov.Imm(asm.R1, 1),
		// 65:	if r1 s> r2 goto +5 <LBB0_7>, JSGTReg dst: r1 off: 5 src: r2
		asm.JSGT.Reg(asm.R1, asm.R2, "LBB0_7"),
		// 66:	goto +0 <LBB0_6>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_6"),
		// 67:	r1 = *(u32 *)(r10 - 36), LdXMemW dst: r1 src: rfp off: -36 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -36, asm.Word).WithSymbol("LBB0_6"),
		// 68:	r2 = *(u64 *)(r10 - 32), LdXMemDW dst: r2 src: rfp off: -32 imm: 0
		asm.LoadMem(asm.R2, asm.RFP, -32, asm.DWord),
		// 69:	*(u32 *)(r2 + 40) = r1, StXMemW dst: r2 src: r1 off: 40 imm: 0
		asm.StoreMem(asm.R2, 40, asm.R1, asm.Word),
		// 70:	goto +0 <LBB0_7>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_7"),
		// 71:	r2 = *(u32 *)(r10 - 40), LdXMemW dst: r2 src: rfp off: -40 imm: 0
		asm.LoadMem(asm.R2, asm.RFP, -40, asm.Word).WithSymbol("LBB0_7"),
		// 72:	r2 <<= 32, LShImm dst: r2 imm: 32
		asm.LSh.Imm(asm.R2, 32),
		// 73:	r2 s>>= 32, ArShImm dst: r2 imm: 32
		asm.ArSh.Imm(asm.R2, 32),
		// 74:	r1 = 1, MovImm dst: r1 imm: 1
		asm.Mov.Imm(asm.R1, 1),
		// 75:	if r1 s> r2 goto +5 <LBB0_9>, JSGTReg dst: r1 off: 5 src: r2
		asm.JSGT.Reg(asm.R1, asm.R2, "LBB0_9"),
		// 76:	goto +0 <LBB0_8>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_8"),
		// 77:	r1 = *(u32 *)(r10 - 40), LdXMemW dst: r1 src: rfp off: -40 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -40, asm.Word).WithSymbol("LBB0_8"),
		// 78:	r2 = *(u64 *)(r10 - 32), LdXMemDW dst: r2 src: rfp off: -32 imm: 0
		asm.LoadMem(asm.R2, asm.RFP, -32, asm.DWord),
		// 79:	*(u32 *)(r2 + 44) = r1, StXMemW dst: r2 src: r1 off: 44 imm: 0
		asm.StoreMem(asm.R2, 44, asm.R1, asm.Word),
		// 80:	goto +0 <LBB0_9>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_9"),
		// 81:	goto +0 <LBB0_10>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_10").WithSymbol("LBB0_9"),
		// 82:	r1 = 0, MovImm dst: r1 imm: 0
		asm.Mov.Imm(asm.R1, 0).WithSymbol("LBB0_10"),
		// 83:	*(u32 *)(r10 - 4) = r1, StXMemW dst: rfp src: r1 off: -4 imm: 0
		asm.StoreMem(asm.RFP, -4, asm.R1, asm.Word),
		// 84:	goto +0 <LBB0_11>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_11"),
		// 85:	r0 = *(u32 *)(r10 - 4), LdXMemW dst: r0 src: rfp off: -4 imm: 0
		asm.LoadMem(asm.R0, asm.RFP, -4, asm.Word).WithSymbol("LBB0_11"),
		// 86:	exit, Exit
		asm.Return(),
	}
}

func __generateKprobeStackProgramSpec(name string) *ebpf.ProgramSpec {
	progSpec := new(ebpf.ProgramSpec)
	progSpec.Name = name
	progSpec.Type = ebpf.Kprobe
	progSpec.License = "GPL"
	__fillKprobeStackInstructions(progSpec)

	return progSpec
}

func main() {
	goflag.Parse()

	glog.Infof("start trace process:'%d' syscalls", _pid)

	calmutils.LoadKallSyms()
	procSyms, _ := calmutils.NewProcSyms(_pid)

	ctx := calmutils.SetupSignalHandler()

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err)
	}

	//-------------------------------------------------------------------------

	// 加载字节码，生成ebpf.CollectionSpec，包括MapSpec和ProgramSpec
	spec, err := bpfmodule.LoadXMTrace()
	if err != nil {
		glog.Fatalf("failed to invoke LoadXMTrace, err: %v", err)
	}

	//-------------------------------------------------------------------------
	// 输出spec中所有的map
	for mapK := range spec.Maps {
		glog.Infof("map name: %s", mapK)
	}

	// 构造.rodata ebpf.Map对象
	rodataMap, err := ebpf.NewMap(spec.Maps[".rodata"])
	if err != nil {
		glog.Fatalf("failed to create .rodata map, err: %v", err)
	}
	defer rodataMap.Close()

	//-------------------------------------------------------------------------

	// rewrite .rodata, bpftool btf dump file xm_trace_syscalls.bpf.o
	err = spec.RewriteConstants(map[string]interface{}{
		"xm_trace_syscall_filter_pid": int32(_pid),
	})
	if err != nil {
		glog.Fatalf("failed to rewrite constants xm_trace_syscall_filter_pid, err: %v", err)
	}

	//-------------------------------------------------------------------------
	// 通过spec构造program和map
	objs := bpfmodule.XMTraceObjects{}
	err = spec.LoadAndAssign(&objs, nil)
	if err != nil {
		glog.Fatalf("failed to load XMTraceObjects, err: %v", err)
	}

	glog.Infof("XmBtfRtpSysEnter name:%s type:%d", objs.XmTraceRawTpSysEnter.String(), objs.XmTraceRawTpSysEnter.Type())

	defer objs.Close()

	//-------------------------------------------------------------------------

	// 创建自定义的ebpf.Program
	kprobeStackProgSpec := __generateKprobeStackProgramSpec("xm_trace_kp__sys_openat")
	// kprobeStackProgSpec.Instructions = make([]asm.Instruction, len(spec.Programs["xm_trace_kp__sys_close"].Instructions))
	// copy(kprobeStackProgSpec.Instructions, spec.Programs["xm_trace_kp__sys_close"].Instructions)

	kprobeStackProgSpec.Instructions.AssociateMap("xm_syscalls_stack_map", objs.XmSyscallsStackMap)
	kprobeStackProgSpec.Instructions.AssociateMap("xm_syscalls_record_map", objs.XmSyscallsRecordMap)
	kprobeStackProgSpec.Instructions.AssociateMap(".rodata", rodataMap)

	kprobeStackProg, err := ebpf.NewProgram(kprobeStackProgSpec)
	if err != nil {
		glog.Fatalf("failed to create kprobeStackProg, err: %v", err)
	}
	defer kprobeStackProg.Close()

	//-------------------------------------------------------------------------

	// 通过reflect来获取Programs的所有成员，统一ebpf Program的attach
	var ebpfLinks []link.Link

	bpfProgs := reflect.Indirect(reflect.ValueOf(objs.XMTracePrograms))
	bpfProgsT := bpfProgs.Type()

	numFields := bpfProgsT.NumField()
	for i := 0; i < numFields; i++ {
		bpfProgS := bpfProgsT.Field(i)                  // reflect.StructField
		bpfProgV := bpfProgs.FieldByName(bpfProgS.Name) // 成员具体值

		// glog.Infof("epbf Prog name[%s] type:[%s] value:[%v] valueType:[%v] kind:%v\n",
		// 	bpfProgS.Name, bpfProgS.Type.String(), bpfProgV, bpfProgV.Type(), bpfProgS.Type.Kind())

		if bpfProgS.Type.Kind() == reflect.Ptr {
			bpfProg := bpfProgV.Interface().(*ebpf.Program)

			progStr := bpfProg.String()
			startPos := strings.Index(progStr, "__")
			endPos := strings.LastIndexByte(progStr, ')')
			progSecSubName := progStr[startPos+2 : endPos]

			glog.Infof("prog:'%s' ebpf info:'%s'", bpfProgS.Name, bpfProg.String())

			switch bpfProg.Type() {
			case ebpf.Kprobe:
				// attach kprobe program
				linkKP, err := link.Kprobe(progSecSubName, bpfProg, nil)
				if err != nil {
					glog.Fatalf("failed to attach %s program for link, error: %v", bpfProg.String(), err)
				} else {
					glog.Infof("attach KProbe %s program for link successed.", bpfProg.String())
					ebpfLinks = append(ebpfLinks, linkKP)
				}
			case ebpf.RawTracepoint:
				linkRawTP, err := link.AttachRawTracepoint(link.RawTracepointOptions{Name: progSecSubName, Program: bpfProg})
				if err != nil {
					glog.Fatalf("failed to attach %s program for link, error: %v", bpfProg.String(), err)
				} else {
					glog.Infof("attach RawTracepoint %s program for link successed.", bpfProg.String())
					ebpfLinks = append(ebpfLinks, linkRawTP)
				}
			case ebpf.Tracing:
				linkTracing, err := link.AttachTracing(link.TracingOptions{Program: bpfProg})
				if err != nil {
					glog.Fatalf("failed to attach %s program for link, error: %v", bpfProg.String(), err)
				} else {
					glog.Infof("attach BTFRawTracepoint %s program for link successed.", bpfProg.String())
					ebpfLinks = append(ebpfLinks, linkTracing)
				}
			default:
				glog.Fatalf("unsupported ebpf type: %d", bpfProg.Type())
			}
		}
	}

	defer func() {
		for i, link := range ebpfLinks {
			glog.Infof("%d link detached", i)
			link.Close()
		}
	}()

	// attach btf raw tracepoint program for link
	// link_sys_enter, err := link.AttachTracing(link.TracingOptions{Program: objs.XmTraceRtpSysEnter})

	// attach raw_tracepoint program
	// link_sys_enter, err := link.AttachRawTracepoint(link.RawTracepointOptions{Name: "sys_enter", Program: objs.XmTraceRawTpSysEnter})
	// if err != nil {
	// 	glog.Fatalf("failed to attach %s program for link, error: %v", objs.XmTraceRawTpSysEnter.String(), err)
	// }
	// defer link_sys_enter.Close()

	// // link_sys_exit, err := link.AttachTracing(link.TracingOptions{Program: objs.XmTraceRtpSysExit})
	// link_sys_exit, err := link.AttachRawTracepoint(link.RawTracepointOptions{Name: "sys_exit", Program: objs.XmTraceRawTpSysExit})
	// if err != nil {
	// 	glog.Fatalf("failed to attach %s program for link, error: %v", objs.XmTraceRawTpSysExit.String(), err)
	// }
	// defer link_sys_exit.Close()

	// link_kp_sys_readlinkat, err := link.Kprobe("sys_readlinkat", objs.XmTraceKpSysReadlinkat, nil)
	// if err != nil {
	// 	glog.Fatalf("failed to attach kprobe %s program for link, error: %v", objs.XmTraceKpSysReadlinkat.String(), err)
	// }
	// defer link_kp_sys_readlinkat.Close()

	// link_kp_sys_openat, err := link.Kprobe("sys_openat", objs.XmTraceKpSysOpenat, nil)
	// if err != nil {
	// 	glog.Fatalf("failed to attach kprobe %s program for link, error: %v", objs.XmTraceKpSysOpenat.String(), err)
	// }
	// defer link_kp_sys_openat.Close()

	// link_kp_sys_close, err := link.Kprobe("sys_close", objs.XmTraceKpSysClose, nil)
	// if err != nil {
	// 	glog.Fatalf("failed to attach kprobe %s program for link, error: %v", objs.XmTraceKpSysClose.String(), err)
	// }
	// defer link_kp_sys_close.Close()

	//-------------------------------------------------------------------------

	evtRD, err := ringbuf.NewReader(objs.XmSyscallsEventMap)
	if err != nil {
		glog.Fatalf("failed to create ringbuf reader, error: %v", err)
	}
	defer evtRD.Close()

	go func() {
		<-ctx.Done()

		if err := evtRD.Close(); err != nil {
			glog.Fatalf("failed to close ringbuf reader, error: %v", err)
		}
	}()

	glog.Infof("Start receiving events...")
	// stackFrameSize := (strconv.IntSize / 8)

	var syscallEvt bpfmodule.XMTraceSyscallEvent
	for {
		record, err := evtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningln("Received signal, exiting...")
				return
			}
			glog.Errorf("reading from ringbuf failed, err: %v", err)
			continue
		}

		// parse the ringbuf record into XMTraceSyscallEvent struct
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, &syscallEvt); err != nil {
			glog.Errorf("failed to parse syscall event, err: %v", err)
			continue
		}

		// float64(syscallEvt.DelayNs)/float64(time.Millisecond)
		glog.Infof("pid:%d, tid:%d, (%f ms) syscall_nr:%d = %d",
			syscallEvt.Pid, syscallEvt.Tid, float64(syscallEvt.CallDelayNs)/float64(time.Millisecond),
			syscallEvt.SyscallNr, syscallEvt.SyscallRet)

		var stackAddrs [_perf_max_stack_depth]uint64

		if syscallEvt.KernelStackId > 0 {
			// stackBytes, err := objs.XmSyscallsStackMap.LookupBytes(syscallEvt.StackId)
			err = objs.XmSyscallsStackMap.Lookup(syscallEvt.KernelStackId, &stackAddrs)
			if err != nil {
				glog.Errorf("failed to lookup stack_id: %d, err: %v", syscallEvt.KernelStackId, err)
			} else {
				// for i := 0; i < _perf_max_stack_depth; i++ {
				for i, stackAddr := range stackAddrs {
					// for i := 0; i < len(stackBytes); i += stackFrameSize {
					// stackAddr := binary.LittleEndian.Uint64(stackBytes[i : i+stackFrameSize])
					if stackAddr == 0 {
						continue
					}
					sym, offset, err := calmutils.FindKsym(stackAddr)
					if err != nil {
						sym = "?"
					}
					glog.Infof("\tip[%d]: 0x%016x\t%s+0x%x", i, stackAddr, sym, offset)
				}
			}
		}

		if syscallEvt.UserStackId > 0 {
			err = objs.XmSyscallsStackMap.Lookup(syscallEvt.UserStackId, &stackAddrs)
			if err != nil {
				glog.Errorf("failed to lookup stack_id: %d, err: %v", syscallEvt.UserStackId, err)
			} else {
				// for i := 0; i < _perf_max_stack_depth; i++ {
				for i, stackAddr := range stackAddrs {
					// for i := 0; i < len(stackBytes); i += stackFrameSize {
					// stackAddr := binary.LittleEndian.Uint64(stackBytes[i : i+stackFrameSize])
					if stackAddr == 0 {
						continue
					}
					sym, offset, moduleName, err := procSyms.FindPsym(stackAddr)
					if err != nil {
						sym = "?"
					}
					glog.Infof("\tip[%d]: 0x%016x\t%s+0x%x [%s]", i, stackAddr, sym, offset, moduleName)
				}
			}
		}

		// objs.XmSyscallsStackMap.Delete(syscallEvt.StackId)
	}

	glog.Infof("trace process syscalls exit")
}
