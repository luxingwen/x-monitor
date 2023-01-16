/*
 * @Author: CALM.WU
 * @Date: 2023-01-16 11:06:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-01-16 11:14:16
 */

package main

import (
	goflag "flag"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/asm"
	"github.com/golang/glog"
)

var __elfFile string

func init() {
	goflag.StringVar(&__elfFile, "elf", "../plugin_ebpf/bpf/.output/xm_trace.bpf.o", "llvm clang generate elf file")
}

func __test_asm_insns() {
	insns := asm.Instructions{
		asm.StoreMem(asm.RFP, -16, asm.R1, asm.DWord),
		asm.LoadMapValue(asm.R1, 0, 64),
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
		asm.LoadMapValue(asm.R1, 0, 0),
		// 14:	r1 = *(u32 *)(r1 + 0), LdXMemW dst: r1 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R1, asm.R1, 0, asm.Word),
		// 15:	if r1 == 0 goto +12 <LBB0_3>, JEqImm dst: r1 off: 12 imm: 0
		asm.JEq.Imm(asm.R1, 0, "LBB0_3"),
		// 16:	goto +0 <LBB0_1>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_1"),
		// 17:	r1 = 0 ll, LoadMapValue dst: r1, fd: 0 off: 0 <.rodata>
		asm.LoadMapValue(asm.R1, 0, 0).WithSymbol("LBB0_1"),
		// 19:	r1 = *(u32 *)(r1 + 0), LdXMemW dst: r1 src: r1 off: 0 imm: 0
		asm.LoadMem(asm.R1, asm.R1, 0, asm.Word),
		// if r1 == 0 goto +10 <LBB0_4>, JEqImm dst: r1 off: 10 imm: 0
		asm.JEq.Imm(asm.R1, 0, "LBB0_4"),
		// 21:	goto +0 <LBB0_2>, JaImm dst: r0 off: 0 imm: 0
		asm.Ja.Label("LBB0_2"),
		// 22:	r1 = *(u32 *)(r10 - 20), LdXMemW dst: r1 src: rfp off: -20 imm: 0
		asm.LoadMem(asm.R1, asm.RFP, -20, asm.Word).WithSymbol("LBB0_2"),
	}

	glog.Info(insns.String())
}

func main() {
	goflag.Parse()

	glog.Infof("start parse elf file:'%s' to ProgramSpec struct", __elfFile)

	// 加载elf文件
	spec, err := ebpf.LoadCollectionSpec(__elfFile)
	if err != nil {
		glog.Fatalf("failed to load collection spec from file: %v", err)
	}

	var firstProgSpec *ebpf.ProgramSpec

	for key, progSpec := range spec.Programs {
		glog.Infof(`progSpec key:'%s', Name:'%s', ProgramType:%d, AttachType:%d, AttachTo:'%s', 
		SectionName:'%s', AttachTarget:%p, Instructions len:%d`,
			key, progSpec.Name, progSpec.Type, progSpec.AttachType,
			progSpec.AttachTo, progSpec.SectionName, progSpec.AttachTarget, len(progSpec.Instructions))

		if firstProgSpec == nil && progSpec.Type == ebpf.Kprobe {
			firstProgSpec = progSpec
		}
	}

	// 输出指令
	// for i, ins := range firstProgSpec.Instructions {
	// 	glog.Infof("%d:\t%#v", i, ins)
	// }

	glog.Info(firstProgSpec.Instructions.String())

	__test_asm_insns()

	glog.Infof("parse elf file:'%s' exit", __elfFile)
}

// llvm-objdump -d -S --no-show-raw-insn --symbolize-operands xm_trace.bpf.o
