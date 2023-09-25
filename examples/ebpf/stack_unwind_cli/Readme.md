1. 编译

   ```
   make stack_unwind_cli VERBOSE=1
   ```

2. 查看fp指令

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗  objdump -S ./stack_unwind_cli |grep -A5 func_c
   0000000000401c37 <func_c>:
   void func_c() {
     401c37:       55                      push   %rbp
     401c38:       48 89 e5                mov    %rsp,%rbp
       debug("%s", "Hello from C\n");
     401c3b:       48 c7 c0 80 62 61 00    mov    $0x616280,%rax
     401c42:       48 8b 00                mov    (%rax),%rax
   --
     401c48:       74 50                   je     401c9a <func_c+0x63>
     401c4a:       48 c7 c0 80 62 61 00    mov    $0x616280,%rax
     401c51:       48 8b 00                mov    (%rax),%rax
     401c54:       48 83 ec 08             sub    $0x8,%rsp
     401c58:       48 8d 15 9d de 00 00    lea    0xde9d(%rip),%rdx        # 40fafc <__dso_handle+0x64>
     401c5f:       52                      push   %rdx
   --
       func_c();
     401d0a:       b8 00 00 00 00          mov    $0x0,%eax
     401d0f:       e8 23 ff ff ff          callq  401c37 <func_c>
   }
     401d14:       90                      nop
     401d15:       c9                      leaveq 
     401d16:       c3                      retq
   ```

3. 使用bpftrace，uprobe + ustack查看用户态堆栈

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗  bpftrace -e 'uprobe:./stack_unwind_cli:func_d {printf("%s",ustack)}' -c ./stack_unwind_cli
   Attaching 1 probe...
   23-09-25 02:37:33:085 [DEBUG] start unwind example
   23-09-25 02:37:33:085 [DEBUG] Hello from A
   
   23-09-25 02:37:33:085 [DEBUG] Hello from B
   
   23-09-25 02:37:33:085 [DEBUG] Hello from C
   
   23-09-25 02:37:33:085 [DEBUG] Hello world from D
   
   
           func_d+0
           func_b+109
           func_a+109
           main+173
           __libc_start_main+243
           0x5541f689495641d7
   ```

   如果关闭了fp，那么bpftrace就无法获取stack

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗ bpftrace -e 'uprobe:./stack_unwind_cli:func_d {printf("%s",ustack)}' -c ./stack_unwind_cli
   Attaching 1 probe...
   23-09-25 07:29:41:154 [DEBUG] start unwind example
   23-09-25 07:29:41:154 [DEBUG] Hello from A
   
   23-09-25 07:29:41:154 [DEBUG] Hello from B
   
   23-09-25 07:29:41:154 [DEBUG] Hello from C
   
   23-09-25 07:29:41:154 [DEBUG] Hello world from D
   ```

4. 使用perf probe、record、script

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗  perf record -e probe_stack_unwind_cli:func_d -aR -g ./stack_unwind_cli 
   23-09-25 02:41:58:171 [DEBUG] start unwind example
   23-09-25 02:41:58:171 [DEBUG] Hello from A
   
   23-09-25 02:41:58:171 [DEBUG] Hello from B
   
   23-09-25 02:41:58:171 [DEBUG] Hello from C
   
   23-09-25 02:41:58:171 [DEBUG] Hello world from D
   
   [ perf record: Woken up 1 times to write data ]
   [ perf record: Captured and wrote 0.294 MB perf.data (1 samples) ]
   ```

   使用perf script打印堆栈

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗ perf script
   stack_unwind_cl 943228 [000] 1714945.044968: probe_stack_unwind_cli:func_d: (401bb6)
                     401bb6 func_d+0x0 (/mnt/Program/x-monitor/bin/stack_unwind_cli)
                     401d14 func_b+0x6d (/mnt/Program/x-monitor/bin/stack_unwind_cli)
                     401d84 func_a+0x6d (/mnt/Program/x-monitor/bin/stack_unwind_cli)
                     401e34 main+0xad (/mnt/Program/x-monitor/bin/stack_unwind_cli)
               7f11acc18493 __libc_start_main+0xf3 (/usr/lib64/libc-2.28.so)
           5541f689495641d7 [unknown] ([unknown])
   ```

5. 读取.debug_frame Section的内容

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗ readelf -w ./stack_unwind_cli 
   Contents of the .eh_frame section:
   
   
   00000000 0000000000000014 00000000 CIE
     Version:               1
     Augmentation:          "zR"
     Code alignment factor: 1
     Data alignment factor: -8
     Return address column: 16
     Augmentation data:     1b
     DW_CFA_def_cfa: r7 (rsp) ofs 8
     DW_CFA_offset: r16 (rip) at cfa-8
     DW_CFA_nop
     DW_CFA_nop
   ......
   ```

6. 读取func_d的FDE信息，可以对比下面objdump -S输出的pc范围，是对应的。

   每个FDE对应一个函数。

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗ readelf  -Wwf ./stack_unwind_cli                               
   Contents of the .eh_frame section:
   
   00000068 000000000000001c 0000006c FDE cie=00000000 pc=0000000000401c60..0000000000401cba
      LOC           CFA      ra    
   0000000000401c60 rsp+8    c-8   
   0000000000401c70 rsp+24   c-8   
   0000000000401c81 rsp+32   c-8   
   0000000000401c97 rsp+40   c-8   
   0000000000401c99 rsp+48   c-8   
   0000000000401ca7 rsp+8    c-8
   ```

7. 反汇编func_d函数

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗ objdump -S ./stack_unwind_cli
   
   0000000000401c60 <func_d>:
     401c60:       48 8b 3d 19 46 21 00    mov    0x214619(%rip),%rdi        # 616280 <g_log_cat>
     401c67:       48 85 ff                test   %rdi,%rdi
     401c6a:       74 44                   je     401cb0 <func_d+0x50>
     401c6c:       48 83 ec 10             sub    $0x10,%rsp
     401c70:       b9 67 fc 40 00          mov    $0x40fc67,%ecx
     401c75:       ba 44 00 00 00          mov    $0x44,%edx
     401c7a:       31 c0                   xor    %eax,%eax
     401c7c:       68 c5 fb 40 00          pushq  $0x40fbc5
     401c81:       41 b9 12 00 00 00       mov    $0x12,%r9d
     401c87:       41 b8 06 00 00 00       mov    $0x6,%r8d
     401c8d:       be 80 fb 40 00          mov    $0x40fb80,%esi
     401c92:       68 62 14 41 00          pushq  $0x411462
     401c97:       6a 14                   pushq  $0x14
     401c99:       e8 e2 24 00 00          callq  404180 <zlog>
     401c9e:       bf 10 27 00 00          mov    $0x2710,%edi
     401ca3:       48 83 c4 28             add    $0x28,%rsp
     401ca7:       e9 24 fe ff ff          jmpq   401ad0 <usleep@plt>
     401cac:       0f 1f 40 00             nopl   0x0(%rax)
     401cb0:       bf 10 27 00 00          mov    $0x2710,%edi
     401cb5:       e9 16 fe ff ff          jmpq   401ad0 <usleep@plt>
     401cba:       66 0f 1f 44 00 00       nopw   0x0(%rax,%rax,1)
   ```

   