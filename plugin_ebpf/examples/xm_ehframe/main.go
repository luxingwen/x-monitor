/*
 * @Author: CALM.WU
 * @Date: 2023-10-07 16:11:53
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-10-17 17:03:08
 */

package main

import (
	"debug/elf"
	"encoding/binary"
	goflag "flag"
	"fmt"
	"os"
	"strings"
	"unsafe"

	dlvFrame "github.com/go-delve/delve/pkg/dwarf/frame"
	"github.com/go-delve/delve/pkg/dwarf/regnum"
	"github.com/golang/glog"
)

type fdeInfo struct {
	begin uint64
	end   uint64
}

type procMapEntry struct {
	module   string
	rip      uint64
	baseAddr uint64
	fdeInfos []fdeInfo
	elfType  elf.Type
}

var (
	__entries = []procMapEntry{
		{"/bin/fio", 0x555deaa8e014, 0x555deaa21000, []fdeInfo{{0x6c8b0, 0x6d56e}, {0x80190, 0x803d1}, {0x7e760, 0x7e86b}}, elf.ET_DYN},
		{module: "./stack_unwind_cli", rip: 0x401ccb, baseAddr: 0x400000, elfType: elf.ET_EXEC},
	}
)

func ptrSizeByRuntimeArch() int {
	return int(unsafe.Sizeof(uintptr(0)))
}

func pointerSize(arch elf.Machine) int {
	//nolint:exhaustive
	switch arch {
	case elf.EM_386:
		return 4
	case elf.EM_AARCH64, elf.EM_X86_64:
		return 8
	default:
		return 0
	}
}

func printFDETable(fde *dlvFrame.FrameDescriptionEntry, order binary.ByteOrder) {
	var strBuilder strings.Builder

	glog.Infof("FDE cie=%04x pc=%08x...%08x", fde.CIE.CIE_id, fde.Begin(), fde.End())

	addFDERowFunc := func(loc uint64, cfa dlvFrame.DWRule, regs map[uint64]dlvFrame.DWRule, retAddrReg uint64) {
		// loc
		strBuilder.WriteString(fmt.Sprintf("LOC: %016x ", loc))
		strBuilder.WriteByte(' ')

		// cfa
		switch cfa.Rule {
		case dlvFrame.RuleCFA:
			strBuilder.WriteString(fmt.Sprintf("CFA: %s+%-5d ", regnum.AMD64ToName(cfa.Reg), cfa.Offset))
		case dlvFrame.RuleExpression:
			fallthrough
		default:
			strBuilder.WriteString(fmt.Sprintf("[CFA:%d] ", cfa.Rule))
		}

		// rbp
		rbp := regs[regnum.AMD64_Rbp]
		switch rbp.Rule {
		case dlvFrame.RuleOffset:
			strBuilder.WriteString(fmt.Sprintf("RBP:c%-5d ", rbp.Offset))
		default:
			strBuilder.WriteString(fmt.Sprintf("%-10s ", "RBP:[u]"))
		}

		// ra
		ra := regs[retAddrReg]
		switch ra.Rule {
		case dlvFrame.RuleOffset:
			strBuilder.WriteString(fmt.Sprintf("RA:c%-5d", ra.Offset))
		default:
			strBuilder.WriteString("RA:[u]")
		}
		glog.Info(strBuilder.String())
		strBuilder.Reset()
	}

	ExecuteDwarfProgram(fde, order, addFDERowFunc)
}

func printFDETables(entry *procMapEntry) {
	glog.Infof("------read .eh_frame section from file:'%s'.", entry.module)

	if _, err := os.Stat(entry.module); err != nil {
		glog.Fatal(err.Error())
	}

	f, err := elf.Open(entry.module)
	if err != nil {
		glog.Fatal(err.Error())
	}
	defer f.Close()

	section := f.Section(".eh_frame")
	data, err := section.Data()
	if err != nil {
		glog.Fatal(err.Error())
	}

	// 得到所有的fde

	fdes, err := dlvFrame.Parse(data, f.ByteOrder, 0, pointerSize(f.Machine), section.Addr)
	if err != nil {
		glog.Error(err.Error())
	} else {
		for _, fde := range fdes {
			if len(entry.fdeInfos) > 0 {
				for _, fi := range entry.fdeInfos {
					if fde.Begin() == fi.begin && fde.End() == fi.end {
						printFDETable(fde, f.ByteOrder)
					}
				}
			} else {
				// 打印全部的fde table
				printFDETable(fde, f.ByteOrder)
			}
		}

		// 打印指定rip的fde table
		var pc uint64
		if entry.elfType == elf.ET_EXEC {
			pc = entry.rip
		} else if entry.elfType == elf.ET_DYN {
			pc = entry.rip - entry.baseAddr
		}
		for _, fde := range fdes {
			if pc >= fde.Begin() && pc < fde.End() {
				glog.Infof("++++++++rip:0x%08x, pc:0x%08x in fde:", entry.rip, pc)
				printFDETable(fde, f.ByteOrder)
			}
		}

		// // procFunc := bi.PCToFunc(pc)
		// // if procFunc != nil {
		// // 	glog.Infof("rip:0x%x ===> procFunc:%s", entry.rip, procFunc.Name)
		// // } else {
		// // 	glog.Errorf("rip:0x%x ===> procFunc is nil", entry.rip)
		// // }

		// fde, err := fdes.FDEForPC(pc)
		// if err != nil {
		// 	glog.Errorf("rip:0x%x, pc:0x%x ===> %s", entry.rip, pc, err.Error())
		// } else {
		// 	glog.Infof("rip:0x%x, pc:0x%x ===> fde{begin:0x%x, end:0x%x, length:0x%x, cie_id:0x%x, cie.Length:0x%x}",
		// 		entry.rip, pc, fde.Begin(), fde.End(), fde.Length, fde.CIE.CIE_id, fde.CIE.Length)

		// 	fctx := fde.EstablishFrame(pc)
		// 	if fctx != nil {
		// 		glog.Infof("rip:0x%x, pc:0x%x ===> frameCtx:%#v",
		// 			entry.rip, pc, fctx)
		// 	}

		// }
	}
}

func main() {
	goflag.Parse()

	for _, e := range __entries {
		printFDETables(&e)
	}
}
