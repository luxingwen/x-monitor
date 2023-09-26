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

   **每个FDE对应一个函数**，这就是函数func_d的FDE。<u>注意栈空间是从高地址向低地址扩展，因此 CFA 相对于 RSP 都是高地址。</u>

   下面输出的使一个FDE的组成，

   00000068：fde的offset，

   000000000000001c：fde的长度，

   FDE cie=00000000：fde所属的cie，

   pc=0000000000401b76..0000000000401b8d：对应函数的起始pc和结束pc。

   ```
   ➜  bin git:(feature-xm-ebpf-collector) ✗ readelf  -wF ./stack_unwind_cli                              
   Contents of the .eh_frame section:
   
   00000068 000000000000001c 0000006c FDE cie=00000000 pc=0000000000401b76..0000000000401b8d
      LOC           CFA      rbp   ra
   0000000000401b76 rsp+8    u     c-8
   0000000000401b77 rsp+16   c-16  c-8
   0000000000401b7a rbp+16   c-16  c-8
   0000000000401b8c rsp+8    c-16  c-8
   ```

   可以根据pc，也就是ip寄存器，得到当前条目。

   CFA：上一级的caller，这要获得rsp寄存器的值。

   保存在堆栈中的通用寄存器：保存过的寄存器都可以从CFA中获取，c = CFA，例如第二行这里就是CFA-16，也就是rsp + 16 - 16

   ra：上一级函数，ra = CFA-8

7. golang生成unwind table结果

   ```
   ➜  utils git:(master) ✗ GO111MODULE=off go test -v -run=TestUnwindTable|grep 401b
       proc_sym_module_test.go:307: 9: &unwind.CompactUnwindTableRow{pc:0x401b76, lrOffset:0, cfaType:0x2, rbpType:0x0, cfaOffset:8, rbpOffset:0}
       proc_sym_module_test.go:307: 10: &unwind.CompactUnwindTableRow{pc:0x401b77, lrOffset:0, cfaType:0x2, rbpType:0x1, cfaOffset:16, rbpOffset:-16}
       proc_sym_module_test.go:307: 11: &unwind.CompactUnwindTableRow{pc:0x401b7a, lrOffset:0, cfaType:0x1, rbpType:0x1, cfaOffset:16, rbpOffset:-16}
       proc_sym_module_test.go:307: 12: &unwind.CompactUnwindTableRow{pc:0x401b8c, lrOffset:0, cfaType:0x2, rbpType:0x1, cfaOffset:8, rbpOffset:-16}
       proc_sym_module_test.go:307: 13: &unwind.CompactUnwindTableRow{pc:0x401b8d, lrOffset:0, cfaType:0x4, rbpType:0x0, cfaOffset:0, rbpOffset:0}
       proc_sym_module_test.go:307: 14: &unwind.CompactUnwindTableRow{pc:0x401b8d, lrOffset:0, cfaType:0x2, rbpType:0x0, cfaOffset:8, rbpOffset:0}
       proc_sym_module_test.go:307: 15: &unwind.CompactUnwindTableRow{pc:0x401b8e, lrOffset:0, cfaType:0x2, rbpType:0x1, cfaOffset:16, rbpOffset:-16}
       proc_sym_module_test.go:307: 16: &unwind.CompactUnwindTableRow{pc:0x401b91, lrOffset:0, cfaType:0x1, rbpType:0x1, cfaOffset:16, rbpOffset:-16}
   
   ```

8. 反汇编test函数，debug版本，objdump -d -S ./stack_unwind_cli

   ```
   0000000000401b76 <test>:
   #include "utils/os.h"
   #include "utils/consts.h"
   #include "utils/strings.h"
   #include "utils/x_ebpf.h"
   
   int test(int x) {
     401b76:       55                      push   %rbp             ---------> 将上一个frame的stack起始地址入栈
     401b77:       48 89 e5                mov    %rsp,%rbp        ---------> 将rsp赋值给rbp，rbp指向了当前frame的stack的起始地址
     401b7a:       89 7d ec                mov    %edi,-0x14(%rbp) ---------> 获取参数，放入-0x14(%rbp) 
       int c = 10;
     401b7d:       c7 45 fc 0a 00 00 00    movl   $0xa,-0x4(%rbp)
       return x * c;
     401b84:       8b 45 ec                mov    -0x14(%rbp),%eax
     401b87:       0f af 45 fc             imul   -0x4(%rbp),%eax
   }
     401b8b:       5d                      pop    %rbp
     401b8c:       c3                      retq
   ```

9. test函数调用方main的汇编

   ```
     401c2f:       48 83 c4 10             add    $0x10,%rsp  ------> 扩大rsp将堆栈向下推，这样，a，b就可以用rbp的偏移来表示，其实还有2字节空余
   	int a, b;
   
       a = 10;
     401c33:       c7 45 f8 0a 00 00 00    movl   $0xa,-0x8(%rbp)
       b = 11;
     401c3a:       c7 45 fc 0b 00 00 00    movl   $0xb,-0x4(%rbp)
     
     
     401c98:       e8 e3 22 00 00          callq  403f80 <zlog>
     401c9d:       48 83 c4 20             add    $0x20,%rsp     ------> 扩展rsp，将堆栈向下推，扩大堆栈，这样
       a = test(a + b);
     401ca1:       8b 55 f8                mov    -0x8(%rbp),%edx
     401ca4:       8b 45 fc                mov    -0x4(%rbp),%eax
     401ca7:       01 d0                   add    %edx,%eax    ---------> +的结果放在%edx
     401ca9:       89 c7                   mov    %eax,%edi    ---------> %edi参数1
     401cab:       e8 c6 fe ff ff          callq  401b76 <test>
     401cb0:       89 45 f8                mov    %eax,-0x8(%rbp) --------> %eax是返回值，a这个变量的地址是-0x8(%rbp)
       debug("hello test(a + b) = %d", a);
     401cb3:       48 c7 c0 80 62 61 00    mov    $0x616280,%rax
     401cba:       48 8b 00                mov    (%rax),%rax
     401cbd:       48 85 c0                test   %rax,%rax
     401cc0:       74 4c                   je     401d0e <main+0x181>
     401cc2:       48 c7 c0 80 62 61 00    mov    $0x616280,%rax
     401cc9:       48 8b 00                mov    (%rax),%rax
     401ccc:       48 83 ec 08             sub    $0x8,%rsp
     401cd0:       8b 55 f8                mov    -0x8(%rbp),%edx
   
   ```

   