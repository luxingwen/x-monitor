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

type procMapEntry struct {
	module     string
	rip        uint64
	baseAddr   uint64
	debugPaths []string
}

var (
	__entries = []procMapEntry{
		{"/bin/fio", 0x555deaa8e014, 0x555deaa21000, []string{"/usr/lib/debug/.build-id/0c/8a9a6540d4d4a8247e07553d72cde921c4379b.debug"}},
		// {"/bin/fio", 0x555deaa88945, 0x555deaa21000, []string{"/usr/lib/debug/.build-id/0c/8a9a6540d4d4a8247e07553d72cde921c4379b.debug"}},
		// {"/usr/lib64/libc-2.28.so", 0x7f6b69274d98, 0x7f6b691ac000, nil},
		// {"/usr/lib64/libc-2.28.so", 0x7f6b69298a79, 0x7f6b691ac000, nil},
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

	glog.Infof("FDE cie=%04x pc=%08x..%08x", fde.CIE.CIE_id, fde.Begin(), fde.End())

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

func unwindStack(entry *procMapEntry) {
	glog.Infof("------read .eh_frame section from file:'%s'.", entry.module)

	if _, err := os.Stat(entry.module); err != nil {
		glog.Fatal(err.Error())
	}

	// bi := proc.NewBinaryInfo(runtime.GOOS, runtime.GOARCH)
	// if err := bi.LoadBinaryInfo(entry.module, entry.baseAddr, entry.debugPaths); err != nil {
	// 	glog.Errorf(err.Error())
	// }

	f, err := elf.Open(entry.module)
	//f, err := os.Open(__binFile)
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
			if (fde.Begin() == 0x6c8b0 && fde.End() == 0x6d56e) ||
				(fde.Begin() == 0x80190 && fde.End() == 0x803d1) ||
				(fde.Begin() == 0x7e760 && fde.End() == 0x7e86b) {
				printFDETable(fde, f.ByteOrder)
			}

		}

		// pc := entry.rip - entry.baseAddr

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
		unwindStack(&e)
	}
}
