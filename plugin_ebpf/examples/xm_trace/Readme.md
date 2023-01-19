# 使用asm编写eBPF Program

- clang编译的program section name是不带 __x64_前缀的。

  ```
   calmwu@localhost  ~/program/cpp_space/x-monitor/plugin_ebpf/bpf/.output  readelf -S ./xm_trace.bpf.o 
  There are 30 section headers, starting at offset 0x4fc0:
  
  Section Headers:
    [ 3] kprobe/sys_readli PROGBITS         0000000000000000  00000040
         00000000000002b8  0000000000000000  AX       0     0     8
    [ 4] .relkprobe/sys_re REL              0000000000000000  00004098
         0000000000000080  0000000000000010          31     3     8
    [ 5] kprobe/sys_openat PROGBITS         0000000000000000  000002f8
         00000000000002b8  0000000000000000  AX       0     0     8
  ```

- 反汇编得到指令

  ```
   calmwu@localhost  ~/program/cpp_space/x-monitor/plugin_ebpf/examples/xm_trace/bpfmodule  llvm-objdump -d -S --no-show-raw-insn  --symbolize-operands ./xmtrace_bpfeb.o
  
  ./xmtrace_bpfeb.o:	file format elf64-bpf
  
  Disassembly of section kprobe/sys_readlinkat:
  
  0000000000000000 <xm_trace_kp__sys_readlinkat>:
  ; // XM_TRACE_KPROBE_PROG(sys_readlinkat)
         0:	r6 = r1
  ;     return bpf_get_current_pid_tgid() >> 32;
         1:	call 14
         2:	r7 = r0
  ;     return bpf_get_current_pid_tgid() & 0xFFFFFFFF;
         3:	call 14
  ; // XM_TRACE_KPROBE_PROG(sys_readlinkat)
         4:	*(u32 *)(r10 - 4) = r0
         5:	r1 = 0 ll
         7:	r2 = *(u32 *)(r1 + 0)
         8:	if r2 == 0 goto +36 <LBB0_8>
         9:	r1 = *(u32 *)(r1 + 0)
        10:	if r1 == 0 goto +5 <LBB0_3>
        11:	r7 >>= 32
  ; // XM_TRACE_KPROBE_PROG(sys_readlinkat)
        12:	r1 = 0 ll
        14:	r1 = *(u32 *)(r1 + 0)
        15:	if r1 != r7 goto +29 <LBB0_8>
  
  0000000000000080 <LBB0_3>:
        16:	r2 = r10
        17:	r2 += -4
  ; // XM_TRACE_KPROBE_PROG(sys_readlinkat)
        18:	r1 = 0 ll
        20:	call 1
        21:	r7 = r0
        22:	if r7 == 0 goto +22 <LBB0_8>
        23:	r1 = r6
        24:	r2 = 0 ll
        26:	r3 = 512
        27:	call 27
        28:	r8 = r0
        29:	r1 = r6
        30:	r2 = 0 ll
        32:	r3 = 768
        33:	call 27
        34:	r2 = r8
        35:	r2 <<= 32
        36:	r2 s>>= 32
        37:	r1 = 1
        38:	if r1 s> r2 goto +1 <LBB0_6>
        39:	*(u32 *)(r7 + 40) = r8
  
  0000000000000140 <LBB0_6>:
        40:	r2 = r0
        41:	r2 <<= 32
        42:	r2 s>>= 32
        43:	if r1 s> r2 goto +1 <LBB0_8>
        44:	*(u32 *)(r7 + 44) = r0
  
  0000000000000168 <LBB0_8>:
        45:	r0 = 0
        46:	exit
  ```

- 使用asm.Instruction去翻译上面的输出，

  ```
  func __fillGetCallStackInstructions(progSpec *ebpf.ProgramSpec) {
  	progSpec.Instructions = asm.Instructions{
  		// 0:	r6 = r1, MovReg dst: r6 src: r1
  		asm.Mov.Reg(asm.R6, asm.R1),
  		// 1:	call 14, Call FnGetCurrentPidTgid
  		asm.FnGetCurrentPidTgid.Call(),
  		// 2:	r7 = r0, MovReg dst: r7 src: r0
  		asm.Mov.Reg(asm.R7, asm.R0),
  		// 3:	call 14, Call FnGetCurrentPidTgid
  		asm.FnGetCurrentPidTgid.Call(),
  		// 4:	*(u32 *)(r10 - 4) = r0, StXMemW dst: rfp src: r0 off: -4 imm: 0
  		asm.StoreMem(asm.RFP, -4, asm.R0, asm.Word),
  		// 5:	r1 = 0 ll, LoadMapValue dst: r1, fd: 0 off: 0 <.rodata>
  		asm.LoadMapValue(asm.R1, 0, 0).WithReference(".rodata"),
  		// 7:	r2 = *(u32 *)(r1 + 0), LdXMemW dst: r2 src: r1 off: 0 imm: 0
  		asm.LoadMem(asm.R2, asm.R1, 0, asm.Word),
  		// 8:	if r2 == 0 goto +36 <LBB1_8>, JEqImm dst: r2 off: 36 imm: 0
  		func() asm.Instruction {
  			ins := asm.JEq.Imm(asm.R2, 0, "") // !! 实际中不需要加上Label，只需要填写Offset
  			ins.Offset = 36
  			return ins
  		}(),
  		// asm.JEq.Imm(asm.R2, 0, "LBB1_8"),
  		// 9:	r1 = *(u32 *)(r1 + 0), LdXMemW dst: r1 src: r1 off: 0 imm: 0
  		asm.LoadMem(asm.R1, asm.R1, 0, asm.Word),
  		// 10:	if r1 == 0 goto +5 <LBB1_3>, JEqImm dst: r1 off: 5 imm: 0
  		func() asm.Instruction {
  			ins := asm.JEq.Imm(asm.R1, 0, "")
  			ins.Offset = 5
  			return ins
  		}(),
  		// 11:	r7 >>= 32, RShImm dst: r7 imm: 32
  		asm.RSh.Imm(asm.R7, 32),
  		// 12:	r1 = 0 ll, LoadMapValue dst: r1, fd: 0 off: 0 <.rodata>
  		asm.LoadMapValue(asm.R1, 0, 0).WithReference(".rodata"),
  		// 14:	r1 = *(u32 *)(r1 + 0), LdXMemW dst: r1 src: r1 off: 0 imm: 0
  		asm.LoadMem(asm.R1, asm.R1, 0, asm.Word),
  		// 15:	if r1 != r7 goto +29 <LBB1_8>, JNEReg dst: r1 off: 29 src: r7
  		func() asm.Instruction {
  			ins := asm.JNE.Reg(asm.R1, asm.R7, "")
  			ins.Offset = 29
  			return ins
  		}(),
  		// 0000000000000080 <LBB1_3>:
  		// 16:	r2 = r10, MovReg dst: r2 src: rfp
  		asm.Mov.Reg(asm.R2, asm.RFP),
  		// 17:	r2 += -4, AddImm dst: r2 imm: -4
  		asm.Add.Imm(asm.R2, -4),
  		// 18:	r1 = 0 ll, LoadMapPtr dst: r1 fd: 0 <xm_syscalls_record_map>
  		asm.LoadMapPtr(asm.R1, 0).WithReference("xm_syscalls_record_map"),
  		// 20:	call 1, Call FnMapLookupElem
  		asm.FnMapLookupElem.Call(),
  		// 21:	r7 = r0, MovReg dst: r7 src: r0
  		asm.Mov.Reg(asm.R7, asm.R0),
  		// 22:	if r7 == 0 goto +22 <LBB1_8>, JEqImm dst: r7 off: 22 imm: 0
  		func() asm.Instruction {
  			ins := asm.JEq.Imm(asm.R7, 0, "")
  			ins.Offset = 22
  			return ins
  		}(),
  		// 23:	r1 = r6, MovReg dst: r1 src: r6
  		asm.Mov.Reg(asm.R1, asm.R6),
  		// 24:	r2 = 0 ll, LoadMapPtr dst: r2 fd: 0 <xm_syscalls_stack_map>
  		asm.LoadMapPtr(asm.R2, 0).WithReference("xm_syscalls_stack_map"),
  		// 26:	r3 = 512, MovImm dst: r3 imm: 512
  		asm.Mov.Imm(asm.R3, 512),
  		// 27:	call 27, Call FnGetStackid
  		asm.FnGetStackid.Call(),
  		// 28:	r8 = r0, MovReg dst: r8 src: r0
  		asm.Mov.Reg(asm.R8, asm.R0),
  		// 29:	r1 = r6, MovReg dst: r1 src: r6
  		asm.Mov.Reg(asm.R1, asm.R6),
  		// 30:	r2 = 0 ll, LoadMapPtr dst: r2 fd: 0 <xm_syscalls_stack_map>
  		asm.LoadMapPtr(asm.R2, 0).WithReference("xm_syscalls_stack_map"),
  		// 32:	r3 = 768, MovImm dst: r3 imm: 768
  		asm.Mov.Imm(asm.R3, 768),
  		// 33:	call 27, Call FnGetStackid
  		asm.FnGetStackid.Call(),
  		// 34:	r2 = r8, MovReg dst: r2 src: r8
  		asm.Mov.Reg(asm.R2, asm.R8),
  		// 35:	r2 <<= 32, LShImm dst: r2 imm: 32
  		asm.LSh.Imm(asm.R2, 32),
  		// 36:	r2 s>>= 32, ArShImm dst: r2 imm: 32
  		asm.ArSh.Imm(asm.R2, 32),
  		// 37:	r1 = 1, MovImm dst: r1 imm: 1
  		asm.Mov.Imm(asm.R1, 1),
  		// 38:	if r1 s> r2 goto +1 <LBB1_6>, JSGTReg dst: r1 off: 1 src: r2
  		func() asm.Instruction {
  			ins := asm.JSGT.Reg(asm.R1, asm.R2, "")
  			ins.Offset = 1
  			return ins
  		}(),
  		// 39:	*(u32 *)(r7 + 40) = r8, StXMemW dst: r7 src: r8 off: 40 imm: 0
  		asm.StoreMem(asm.R7, 40, asm.R8, asm.Word),
  		// 0000000000000140 <LBB1_6>:
  		// 40:	r2 = r0, MovReg dst: r2 src: r0
  		asm.Mov.Reg(asm.R2, asm.R0),
  		// 41:	r2 <<= 32, LShImm dst: r2 imm: 32
  		asm.LSh.Imm(asm.R2, 32),
  		// 42:	r2 s>>= 32, ArShImm dst: r2 imm: 32
  		asm.ArSh.Imm(asm.R2, 32),
  		// 43:	if r1 s> r2 goto +1 <LBB1_8>, JSGTReg dst: r1 off: 1 src: r2
  		func() asm.Instruction {
  			ins := asm.JSGT.Reg(asm.R1, asm.R2, "")
  			ins.Offset = 1
  			return ins
  		}(),
  		// 44:	*(u32 *)(r7 + 44) = r0, StXMemW dst: r7 src: r0 off: 44 imm: 0
  		asm.StoreMem(asm.R7, 44, asm.R0, asm.Word),
  		// 0000000000000168 <LBB1_8>:
  		// 45:	r0 = 0, MovImm dst: r0 imm: 0
  		asm.Mov.Imm(asm.R0, 0),
  		// 46:	exit, Exit
  		asm.Return(),
  	}
  }
  ```

- 打印asm.Instructions

  ```
  glog.Info("---\n", progSpec.Instructions.String())
  ```

- bpftool dump prog instructions

  ```
   ⚡ root@localhost  ~  bpftool prog list                                                                                    
  2586: raw_tracepoint  name xm_trace_raw_tp  tag d7be4ae90a2d7c31  gpl
  	loaded_at 2023-01-19T16:35:04+0800  uid 0
  	xlated 320B  jited 185B  memlock 4096B  map_ids 1753,1754
  	btf_id 719
  	pids xm_trace(843060)
  2587: raw_tracepoint  name xm_trace_raw_tp  tag 1397a028d2cd7a6c  gpl
  	loaded_at 2023-01-19T16:35:04+0800  uid 0
  	xlated 536B  jited 313B  memlock 4096B  map_ids 1753,1754,1755
  	btf_id 719
  	pids xm_trace(843060)
  2588: kprobe  name sys_close  tag 7d80c44c86509e56  gpl
  	loaded_at 2023-01-19T16:35:04+0800  uid 0
  	xlated 392B  jited 240B  memlock 4096B  map_ids 1752,1754,1757
  	pids xm_trace(843060)
  2589: kprobe  name sys_readlinkat  tag 7d80c44c86509e56  gpl
  	loaded_at 2023-01-19T16:35:04+0800  uid 0
  	xlated 392B  jited 240B  memlock 4096B  map_ids 1752,1754,1757
  	pids xm_trace(843060)
  2590: kprobe  name sys_openat  tag 7d80c44c86509e56  gpl
  	loaded_at 2023-01-19T16:35:04+0800  uid 0
  	xlated 392B  jited 240B  memlock 4096B  map_ids 1752,1754,1757
  	pids xm_trace(843060)
  ```

  dump

  ```
   ✘ ⚡ root@localhost  ~  bpftool prog dump xlated id 2590     
     0: (bf) r6 = r1
     1: (85) call bpf_get_current_pid_tgid#124560
     2: (bf) r7 = r0
     3: (85) call bpf_get_current_pid_tgid#124560
     4: (63) *(u32 *)(r10 -4) = r0
     5: (18) r1 = map[id:1752][0]+0
     7: (61) r2 = *(u32 *)(r1 +0)
     8: (15) if r2 == 0x0 goto pc+38
     9: (61) r1 = *(u32 *)(r1 +0)
    10: (15) if r1 == 0x0 goto pc+5
    11: (77) r7 >>= 32
    12: (18) r1 = map[id:1752][0]+0
    14: (61) r1 = *(u32 *)(r1 +0)
    15: (5d) if r1 != r7 goto pc+31
    16: (bf) r2 = r10
    17: (07) r2 += -4
    18: (18) r1 = map[id:1754]
    20: (85) call __htab_map_lookup_elem#138112
    21: (15) if r0 == 0x0 goto pc+1
    ....
  ```

  

  

