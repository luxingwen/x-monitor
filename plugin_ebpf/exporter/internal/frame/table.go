/*
 * @Author: CALM.WU
 * @Date: 2023-10-24 14:47:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-11-03 15:27:11
 */

package frame

import (
	"bytes"
	"debug/elf"
	"encoding/binary"
	"fmt"
	"math"

	dlvFrame "github.com/go-delve/delve/pkg/dwarf/frame"
	"github.com/go-delve/delve/pkg/dwarf/leb128"
	"github.com/go-delve/delve/pkg/dwarf/regnum"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"golang.org/x/exp/slices"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

const (
	__maxPerModuleFDETableCount     = 8192        // 每个 module 包含的 fde table 最大数量
	__maxPerProcessAssocModuleCount = 56          // 每个进程关联的 module 最大数量
	__maxPerModuleAssocFDERowCount  = (45 * 1024) // 每个 module 关联的 fde row 最大数量
	__DW_CFA_GNU_args_size          = 0x2e
)

type FrameContext struct {
	loc             uint64
	order           binary.ByteOrder
	address         uint64
	CFA             dlvFrame.DWRule
	Regs            map[uint64]dlvFrame.DWRule
	initialRegs     map[uint64]dlvFrame.DWRule
	buf             *bytes.Buffer
	cie             *dlvFrame.CommonInformationEntry
	RetAddrReg      uint64
	codeAlignment   uint64
	dataAlignment   int64
	rememberedState *stateStack
}

type rowState struct {
	cfa  dlvFrame.DWRule
	regs map[uint64]dlvFrame.DWRule
}

type instruction func(frame *FrameContext)

var fnlookup = map[byte]instruction{
	dlvFrame.DW_CFA_advance_loc:        advanceloc,
	dlvFrame.DW_CFA_offset:             offset,
	dlvFrame.DW_CFA_restore:            restore,
	dlvFrame.DW_CFA_set_loc:            setloc,
	dlvFrame.DW_CFA_advance_loc1:       advanceloc1,
	dlvFrame.DW_CFA_advance_loc2:       advanceloc2,
	dlvFrame.DW_CFA_advance_loc4:       advanceloc4,
	dlvFrame.DW_CFA_offset_extended:    offsetextended,
	dlvFrame.DW_CFA_restore_extended:   restoreextended,
	dlvFrame.DW_CFA_undefined:          undefined,
	dlvFrame.DW_CFA_same_value:         samevalue,
	dlvFrame.DW_CFA_register:           register,
	dlvFrame.DW_CFA_remember_state:     rememberstate,
	dlvFrame.DW_CFA_restore_state:      restorestate,
	dlvFrame.DW_CFA_def_cfa:            defcfa,
	dlvFrame.DW_CFA_def_cfa_register:   defcfaregister,
	dlvFrame.DW_CFA_def_cfa_offset:     defcfaoffset,
	dlvFrame.DW_CFA_def_cfa_expression: defcfaexpression,
	dlvFrame.DW_CFA_expression:         expression,
	dlvFrame.DW_CFA_offset_extended_sf: offsetextendedsf,
	dlvFrame.DW_CFA_def_cfa_sf:         defcfasf,
	dlvFrame.DW_CFA_def_cfa_offset_sf:  defcfaoffsetsf,
	dlvFrame.DW_CFA_val_offset:         valoffset,
	dlvFrame.DW_CFA_val_offset_sf:      valoffsetsf,
	dlvFrame.DW_CFA_val_expression:     valexpression,
	dlvFrame.DW_CFA_lo_user:            louser,
	dlvFrame.DW_CFA_hi_user:            hiuser,
	__DW_CFA_GNU_args_size:             gnuargsize,
}

const low_6_offset = 0x3f

// stateStack is a stack where `DW_CFA_remember_state` pushes
// its CFA and registers state and `DW_CFA_restore_state`
// pops them.
type stateStack struct {
	items []rowState
}

func newStateStack() *stateStack {
	return &stateStack{
		items: make([]rowState, 0),
	}
}

func (stack *stateStack) push(state rowState) {
	stack.items = append(stack.items, state)
}

func (stack *stateStack) pop() rowState {
	restored := stack.items[len(stack.items)-1]
	stack.items = stack.items[0 : len(stack.items)-1]
	return restored
}

func executeCIEInstructions(cie *dlvFrame.CommonInformationEntry) *FrameContext {
	initialInstructions := make([]byte, len(cie.InitialInstructions))
	copy(initialInstructions, cie.InitialInstructions)
	frame := &FrameContext{
		cie:             cie,
		Regs:            make(map[uint64]dlvFrame.DWRule),
		RetAddrReg:      cie.ReturnAddressRegister,
		initialRegs:     make(map[uint64]dlvFrame.DWRule),
		codeAlignment:   cie.CodeAlignmentFactor,
		dataAlignment:   cie.DataAlignmentFactor,
		buf:             bytes.NewBuffer(initialInstructions),
		rememberedState: newStateStack(),
	}

	frame.executeDwarfProgram()
	return frame
}

type receiveFDERowFunc func(loc uint64, cfa dlvFrame.DWRule, regs map[uint64]dlvFrame.DWRule, retAddrReg uint64)

func ExecuteDwarfProgram(fde *dlvFrame.FrameDescriptionEntry, order binary.ByteOrder, recevieRow receiveFDERowFunc) {
	frame := executeCIEInstructions(fde.CIE)
	frame.order = order
	frame.loc = fde.Begin()
	frame.buildFDETables(fde.Instructions, recevieRow)
}

func (frame *FrameContext) buildFDETables(instructions []byte, recevieRow receiveFDERowFunc) {
	frame.buf.Truncate(0)
	frame.buf.Write(instructions)

	var prevLoc uint64 = frame.loc
	rowID := 0

	for frame.buf.Len() > 0 {
		executeDwarfInstruction(frame)

		if frame.loc != prevLoc {
			// start a new frame
			//glog.Infof("fde row:%d, loc:0x%x, CFA:%#v, Regs:%#v, RetAddrReg:%#v", rowID, prevLoc, frame.CFA, frame.Regs, frame.RetAddrReg)
			if recevieRow != nil {
				recevieRow(prevLoc, frame.CFA, frame.Regs, frame.RetAddrReg)
			}
			rowID += 1
			prevLoc = frame.loc
		}
	}
	//glog.Infof("fde row:%d, loc:0x%x, CFA:%#v, Regs:%#v, RetAddrReg:%#v", rowID, prevLoc, frame.CFA, frame.Regs, frame.RetAddrReg)
	if recevieRow != nil {
		recevieRow(prevLoc, frame.CFA, frame.Regs, frame.RetAddrReg)
	}
}

func (frame *FrameContext) executeDwarfProgram() {
	for frame.buf.Len() > 0 {
		executeDwarfInstruction(frame)
	}
}

func executeDwarfInstruction(frame *FrameContext) {
	instruction, err := frame.buf.ReadByte()
	if err != nil {
		panic("Could not read from instruction buffer")
	}

	if instruction == dlvFrame.DW_CFA_nop {
		return
	}

	fn := lookupFunc(instruction, frame.buf)

	fn(frame)
}

func lookupFunc(instruction byte, buf *bytes.Buffer) instruction {
	const high_2_bits = 0xc0
	var restore bool

	// Special case the 3 opcodes that have their argument encoded in the opcode itself.
	switch instruction & high_2_bits {
	case dlvFrame.DW_CFA_advance_loc:
		instruction = dlvFrame.DW_CFA_advance_loc
		restore = true

	case dlvFrame.DW_CFA_offset:
		instruction = dlvFrame.DW_CFA_offset
		restore = true

	case dlvFrame.DW_CFA_restore:
		instruction = dlvFrame.DW_CFA_restore
		restore = true
	}

	if restore {
		// Restore the last byte as it actually contains the argument for the opcode.
		err := buf.UnreadByte()
		if err != nil {
			panic("Could not unread byte")
		}
	}

	fn, ok := fnlookup[instruction]
	if !ok {
		panic(fmt.Sprintf("Encountered an unexpected DWARF CFA opcode: %#v", instruction))
	}

	return fn
}

func advanceloc(frame *FrameContext) {
	b, err := frame.buf.ReadByte()
	if err != nil {
		panic("Could not read byte")
	}

	delta := b & low_6_offset
	frame.loc += uint64(delta) * frame.codeAlignment
	//glog.Infof("advanceloc loc:0x%x", frame.loc)
}

func advanceloc1(frame *FrameContext) {
	delta, err := frame.buf.ReadByte()
	if err != nil {
		panic("Could not read byte")
	}

	frame.loc += uint64(delta) * frame.codeAlignment
	//glog.Infof("advanceloc1 loc:0x%x", frame.loc)
}

func advanceloc2(frame *FrameContext) {
	var delta uint16
	binary.Read(frame.buf, frame.order, &delta)

	//glog.Infof("advanceloc2 frame.loc:0x%x, delta:%d, codeAlignment:%d",
	//	frame.loc, delta, frame.codeAlignment)

	frame.loc += uint64(delta) * frame.codeAlignment
	//glog.Infof("advanceloc2 loc:0x%x", frame.loc)
}

func advanceloc4(frame *FrameContext) {
	var delta uint32
	binary.Read(frame.buf, frame.order, &delta)

	frame.loc += uint64(delta) * frame.codeAlignment
	//glog.Infof("advanceloc4 loc:0x%x", frame.loc)
}

func offset(frame *FrameContext) {
	b, err := frame.buf.ReadByte()
	if err != nil {
		panic(err)
	}

	var (
		reg       = b & low_6_offset
		offset, _ = leb128.DecodeUnsigned(frame.buf)
	)

	frame.Regs[uint64(reg)] = dlvFrame.DWRule{Offset: int64(offset) * frame.dataAlignment, Rule: dlvFrame.RuleOffset}
}

func restore(frame *FrameContext) {
	b, err := frame.buf.ReadByte()
	if err != nil {
		panic(err)
	}

	reg := uint64(b & low_6_offset)
	oldrule, ok := frame.initialRegs[reg]
	if ok {
		frame.Regs[reg] = dlvFrame.DWRule{Offset: oldrule.Offset, Rule: dlvFrame.RuleOffset}
	} else {
		frame.Regs[reg] = dlvFrame.DWRule{Rule: dlvFrame.RuleUndefined}
	}
}

func setloc(frame *FrameContext) {
	var loc uint64
	binary.Read(frame.buf, frame.order, &loc)

	frame.loc = loc //** + frame.cie.staticBase, staticBase is 0, frame.Parse third argument
	//glog.Infof("setloc loc:0x%x", frame.loc)
}

func offsetextended(frame *FrameContext) {
	var (
		reg, _    = leb128.DecodeUnsigned(frame.buf)
		offset, _ = leb128.DecodeUnsigned(frame.buf)
	)

	frame.Regs[reg] = dlvFrame.DWRule{Offset: int64(offset) * frame.dataAlignment, Rule: dlvFrame.RuleOffset}
}

func undefined(frame *FrameContext) {
	reg, _ := leb128.DecodeUnsigned(frame.buf)
	frame.Regs[reg] = dlvFrame.DWRule{Rule: dlvFrame.RuleUndefined}
}

func samevalue(frame *FrameContext) {
	reg, _ := leb128.DecodeUnsigned(frame.buf)
	frame.Regs[reg] = dlvFrame.DWRule{Rule: dlvFrame.RuleSameVal}
}

func register(frame *FrameContext) {
	reg1, _ := leb128.DecodeUnsigned(frame.buf)
	reg2, _ := leb128.DecodeUnsigned(frame.buf)
	frame.Regs[reg1] = dlvFrame.DWRule{Reg: reg2, Rule: dlvFrame.RuleRegister}
}

func rememberstate(frame *FrameContext) {
	clonedRegs := make(map[uint64]dlvFrame.DWRule, len(frame.Regs))
	for k, v := range frame.Regs {
		clonedRegs[k] = v
	}
	frame.rememberedState.push(rowState{cfa: frame.CFA, regs: clonedRegs})
}

func restorestate(frame *FrameContext) {
	restored := frame.rememberedState.pop()

	frame.CFA = restored.cfa
	frame.Regs = restored.regs
}

func restoreextended(frame *FrameContext) {
	reg, _ := leb128.DecodeUnsigned(frame.buf)

	oldrule, ok := frame.initialRegs[reg]
	if ok {
		frame.Regs[reg] = dlvFrame.DWRule{Offset: oldrule.Offset, Rule: dlvFrame.RuleOffset}
	} else {
		frame.Regs[reg] = dlvFrame.DWRule{Rule: dlvFrame.RuleUndefined}
	}
}

func defcfa(frame *FrameContext) {
	reg, _ := leb128.DecodeUnsigned(frame.buf)
	offset, _ := leb128.DecodeUnsigned(frame.buf)

	frame.CFA.Rule = dlvFrame.RuleCFA
	frame.CFA.Reg = reg
	frame.CFA.Offset = int64(offset)
}

func defcfaregister(frame *FrameContext) {
	reg, _ := leb128.DecodeUnsigned(frame.buf)
	frame.CFA.Reg = reg
}

func defcfaoffset(frame *FrameContext) {
	offset, _ := leb128.DecodeUnsigned(frame.buf)
	frame.CFA.Offset = int64(offset)
}

func defcfasf(frame *FrameContext) {
	reg, _ := leb128.DecodeUnsigned(frame.buf)
	offset, _ := leb128.DecodeSigned(frame.buf)

	frame.CFA.Rule = dlvFrame.RuleCFA
	frame.CFA.Reg = reg
	frame.CFA.Offset = offset * frame.dataAlignment
}

func defcfaoffsetsf(frame *FrameContext) {
	offset, _ := leb128.DecodeSigned(frame.buf)
	offset *= frame.dataAlignment
	frame.CFA.Offset = offset
}

func defcfaexpression(frame *FrameContext) {
	var (
		l, _ = leb128.DecodeUnsigned(frame.buf)
		expr = frame.buf.Next(int(l))
	)

	frame.CFA.Expression = expr
	frame.CFA.Rule = dlvFrame.RuleExpression
}

func expression(frame *FrameContext) {
	var (
		reg, _ = leb128.DecodeUnsigned(frame.buf)
		l, _   = leb128.DecodeUnsigned(frame.buf)
		expr   = frame.buf.Next(int(l))
	)

	frame.Regs[reg] = dlvFrame.DWRule{Rule: dlvFrame.RuleExpression, Expression: expr}
}

func offsetextendedsf(frame *FrameContext) {
	var (
		reg, _    = leb128.DecodeUnsigned(frame.buf)
		offset, _ = leb128.DecodeSigned(frame.buf)
	)

	frame.Regs[reg] = dlvFrame.DWRule{Offset: offset * frame.dataAlignment, Rule: dlvFrame.RuleOffset}
}

func valoffset(frame *FrameContext) {
	var (
		reg, _    = leb128.DecodeUnsigned(frame.buf)
		offset, _ = leb128.DecodeUnsigned(frame.buf)
	)

	frame.Regs[reg] = dlvFrame.DWRule{Offset: int64(offset), Rule: dlvFrame.RuleValOffset}
}

func valoffsetsf(frame *FrameContext) {
	var (
		reg, _    = leb128.DecodeUnsigned(frame.buf)
		offset, _ = leb128.DecodeSigned(frame.buf)
	)

	frame.Regs[reg] = dlvFrame.DWRule{Offset: offset * frame.dataAlignment, Rule: dlvFrame.RuleValOffset}
}

func valexpression(frame *FrameContext) {
	var (
		reg, _ = leb128.DecodeUnsigned(frame.buf)
		l, _   = leb128.DecodeUnsigned(frame.buf)
		expr   = frame.buf.Next(int(l))
	)

	frame.Regs[reg] = dlvFrame.DWRule{Rule: dlvFrame.RuleValExpression, Expression: expr}
}

func louser(frame *FrameContext) {
	frame.buf.Next(1)
}

func hiuser(frame *FrameContext) {
	frame.buf.Next(1)
}

func gnuargsize(frame *FrameContext) {
	// The DW_CFA_GNU_args_size instruction takes an unsigned LEB128 operand representing an argument size.
	// Just read and do nothing.
	_, _ = leb128.DecodeSigned(frame.buf)
}

func CreatePidMaps(pid int32) (*bpfmodule.XMProfileXmProfilePidMaps, []*calmutils.ProcMapsModule, error) {
	var (
		modules []*calmutils.ProcMapsModule
		err     error
	)
	procMaps := new(bpfmodule.XMProfileXmProfilePidMaps)

	if modules, err = GetProcModules(pid); err != nil {
		return nil, nil, errors.Wrap(err, "get process modules failed.")
	} else {

		for i, module := range modules {
			procMaps.Modules[i].StartAddr = module.StartAddr
			procMaps.Modules[i].EndAddr = module.EndAddr
			procMaps.Modules[i].BuildIdHash = calmutils.HashStr2Uint64(module.BuildID)
			procMaps.Modules[i].Type = uint32(module.Type)

			glog.Infof("Pid:%d maps module:'%s' buildID:'%s' buidIDHash:%d startAddr:%#x endAddr:%#x",
				pid, module.Pathname, module.BuildID, procMaps.Modules[i].BuildIdHash, module.StartAddr, module.EndAddr)

			procMaps.ModuleCount += 1
			if procMaps.ModuleCount >= __maxPerProcessAssocModuleCount {
				err = errors.Errorf("/proc/%d/maps modules count exceeds the limit:%d", pid, __maxPerProcessAssocModuleCount)
				return nil, nil, err
			}
		}
	}

	return procMaps, modules, nil
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

func CreateModuleFDETables(modulePath string) (*bpfmodule.XMProfileXmProfileModuleFdeTables, error) {
	// 在创建 module fde tables 之前要判断是否已经存在于 ebpf hash map 中

	ef, err := elf.Open(modulePath)
	if err != nil {
		err = errors.Wrap(err, "use elf open module.")
		glog.Error(err.Error())
		return nil, err
	}
	defer ef.Close()

	section := ef.Section(".eh_frame")
	if section == nil {
		return nil, errors.Errorf("module:'%s' no .eh_frame section", modulePath)
	}

	data, err := section.Data()
	if err != nil {
		err = errors.Wrap(err, "get .eh_frame section data.")
		glog.Error(err.Error())
		return nil, err
	}

	fdes, err := dlvFrame.Parse(data, ef.ByteOrder, 0, pointerSize(ef.Machine), section.Addr)
	if err != nil {
		err = errors.Wrap(err, "delve frame parse .eh_frame section data.")
		glog.Error(err.Error())
		return nil, err
	}

	procModuleFDETables := new(bpfmodule.XMProfileXmProfileModuleFdeTables)
	procModuleFDETables.RefCount = 1
	procModuleFDETables.FdeTableCount = 0

	moduleAssocFDERowCount := 0
	var innerErr error = nil

	// 轮询所有的 fde
	for _, fde := range fdes {
		if procModuleFDETables.FdeTableCount >= __maxPerModuleFDETableCount {
			glog.Warningf("module:'%s' fde table count exceeds the limit, ignore the rest.", modulePath)
			break
		}

		tableInfo := &procModuleFDETables.FdeInfos[procModuleFDETables.FdeTableCount]
		tableInfo.Start = fde.Begin()
		tableInfo.End = fde.End()
		tableInfo.RowCount = int32(0)
		tableInfo.RowPos = int32(moduleAssocFDERowCount)

		innerErr = nil

		ExecuteDwarfProgram(fde, ef.ByteOrder, func(loc uint64, cfa dlvFrame.DWRule, regs map[uint64]dlvFrame.DWRule, retAddrReg uint64) {
			if innerErr != nil {
				return
			}

			// 每个 fde table 最多 row 数
			if moduleAssocFDERowCount >= __maxPerModuleAssocFDERowCount {
				innerErr = errors.Errorf("module:'%s' row count exceeds the limit:%d. so fde table{start:'%#x---end:%#x'} ignore.",
					modulePath, __maxPerModuleAssocFDERowCount, tableInfo.Start, tableInfo.End)
				//glog.Error(innerErr.Error())
				return
			} else {
				tableRow := &procModuleFDETables.FdeRows[moduleAssocFDERowCount]
				tableRow.Loc = loc
				// 只支持 cfg 计算的寄存器是 rsp 或 rbp，如果使用其它寄存器放弃创建 FDETables
				if cfa.Reg != regnum.AMD64_Rsp && cfa.Reg != regnum.AMD64_Rbp {
					innerErr = errors.Errorf("module:'%s' fde table{start:'%#x---end:%#x'} row:%d has invalid reg:'%s'",
						modulePath, tableInfo.Start, tableInfo.End, tableInfo.RowCount, regnum.AMD64ToName(cfa.Reg))
					//glog.Error(innerErr.Error())
					return
				}
				// cfa 的获取方式
				tableRow.Cfa.Reg = cfa.Reg // cfa 获取的寄存器号
				tableRow.Cfa.Offset = cfa.Offset
				// rbp 寄存器
				tableRow.RbpCfaOffset = math.MaxInt32 // 约定用 maxInt32 标识没有获取到
				if regs[regnum.AMD64_Rbp].Rule == dlvFrame.RuleOffset {
					tableRow.RbpCfaOffset = int32(regs[regnum.AMD64_Rbp].Offset) // RuleOffset
				}
				// ra
				tableRow.RaCfaOffset = math.MaxInt32
				if regs[retAddrReg].Rule == dlvFrame.RuleOffset {
					tableRow.RaCfaOffset = int32(regs[retAddrReg].Offset)
				}
				tableInfo.RowCount += 1
				moduleAssocFDERowCount += 1
			}
		})

		if innerErr == nil && tableInfo.RowCount > 0 {
			// procModuleFDETables = nil
			// return nil, innerErr
			procModuleFDETables.FdeTableCount++
		} else {
			if err != nil {
				glog.Errorf(innerErr.Error())
			}
			if tableInfo.RowCount == 0 {
				glog.Warningf("module:'%s' fde table{start:'%#x---end:%#x'} is empty", modulePath, tableInfo.Start, tableInfo.End)
			}
		}
	}
	// 按 table 的 start 排序
	slices.SortFunc(procModuleFDETables.FdeInfos[0:procModuleFDETables.FdeTableCount],
		func(a, b bpfmodule.XMProfileXmProfileFdeTableInfo) bool { return a.Start < b.Start })

	glog.Infof("module:'%s' have %d FDETables, %d FDERows", modulePath, procModuleFDETables.FdeTableCount, moduleAssocFDERowCount)
	return procModuleFDETables, nil
}
