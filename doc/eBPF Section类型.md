# eBPF Section类型

## SEC宏

### 宏定义

每个ebpf函数都会添加SEC这个函数前缀，SEC宏定义

```
#define SEC(name) __attribute__((section(name), used))
```

### 宏输出

这个宏的目的是在elf文件中添加对应的Section，如下执行readelf命令会看到

```
 ⚡ root@localhost  /home/calmwu/program/cpp_space/x-monitor/plugin_ebpf/bpf/.output   main ±  readelf -S xm_runqlat.bpf.o 
There are 13 section headers, starting at offset 0x608:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000000000  00000040
       0000000000000000  0000000000000000  AX       0     0     4
  [ 2] tp_btf/sched_wake PROGBITS         0000000000000000  00000040
       0000000000000010  0000000000000000  AX       0     0     8
  [ 3] tp_btf/sched_wake PROGBITS         0000000000000000  00000050
       0000000000000010  0000000000000000  AX       0     0     8
  [ 4] tp_btf/sched_swit PROGBITS         0000000000000000  00000060
       0000000000000010  0000000000000000  AX       0     0     8
  [ 5] license           PROGBITS         0000000000000000  00000070
```

### SEC中的内容

文件libbpf.c中定义了SEC参数内容，带有+号的意味前缀。

```
static const struct bpf_sec_def section_defs[] = {
	SEC_DEF("socket",		SOCKET_FILTER, 0, SEC_NONE | SEC_SLOPPY_PFX),
	SEC_DEF("sk_reuseport/migrate",	SK_REUSEPORT, BPF_SK_REUSEPORT_SELECT_OR_MIGRATE, SEC_ATTACHABLE | SEC_SLOPPY_PFX),
	SEC_DEF("sk_reuseport",		SK_REUSEPORT, BPF_SK_REUSEPORT_SELECT, SEC_ATTACHABLE | SEC_SLOPPY_PFX),
	SEC_DEF("kprobe+",		KPROBE,	0, SEC_NONE, attach_kprobe),
	SEC_DEF("uprobe+",		KPROBE,	0, SEC_NONE, attach_uprobe),
	SEC_DEF("kretprobe+",		KPROBE, 0, SEC_NONE, attach_kprobe),
	SEC_DEF("uretprobe+",		KPROBE, 0, SEC_NONE, attach_uprobe),
	SEC_DEF("kprobe.multi+",	KPROBE,	BPF_TRACE_KPROBE_MULTI, SEC_NONE, attach_kprobe_multi),
	SEC_DEF("kretprobe.multi+",	KPROBE,	BPF_TRACE_KPROBE_MULTI, SEC_NONE, attach_kprobe_multi),
	SEC_DEF("usdt+",		KPROBE,	0, SEC_NONE, attach_usdt),
	SEC_DEF("tc",			SCHED_CLS, 0, SEC_NONE),
	SEC_DEF("classifier",		SCHED_CLS, 0, SEC_NONE | SEC_SLOPPY_PFX | SEC_DEPRECATED),
	SEC_DEF("action",		SCHED_ACT, 0, SEC_NONE | SEC_SLOPPY_PFX),
	SEC_DEF("tracepoint+",		TRACEPOINT, 0, SEC_NONE, attach_tp),
	SEC_DEF("tp+",			TRACEPOINT, 0, SEC_NONE, attach_tp),
	SEC_DEF("raw_tracepoint+",	RAW_TRACEPOINT, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("raw_tp+",		RAW_TRACEPOINT, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("raw_tracepoint.w+",	RAW_TRACEPOINT_WRITABLE, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("raw_tp.w+",		RAW_TRACEPOINT_WRITABLE, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("tp_btf+",		TRACING, BPF_TRACE_RAW_TP, SEC_ATTACH_BTF, attach_trace),
	......
```

## SEC的函数编写

编写最多的是kprobe、tracepoint、btf raw tracepoint的函数，为了简便函数定义和参数读取，下面分类描述

### tracepoint enter函数

- 参数：struct trace_event_raw_sys_enter *ctx，方便读取前6个参数

  ```
  struct trace_event_raw_sys_enter {
  	struct trace_entry ent;
  	long int id;
  	long unsigned int args[6];
  	char __data[0];
  };
  ```

  当然也可以按查看debugfs的mount路径，找到对应的tracing/events/xxx/tp_name/format文件来定义函数结构体，只是不怎么通用。

  读取参数方式

  - int

    ```
    unsigned int fd = (unsigned int)ctx->args[0];
    ```

  - string/buff

    ```
    bpf_probe_read_user(&check_filename, filename_len, (char*)ctx->args[1]);
    ```

tracepoint exit函数

- 参数：struct trace_event_raw_sys_exit *ctx

  读取返回值

  ```
  unsigned int fd = (unsigned int)ctx->ret;
  ```

### kprobe函数

函数定义：int BPF_KPROBE(函数名, 内核函数参数)，BPF_KPROBE宏实际是展开了struct pt_regs *ctx（Registers and BPF context）。example如下

```
SEC("kprobe/inet_listen")
int BPF_KPROBE(inet_listen_entry, struct socket *sock, int backlog)
{
	__u64 pid_tgid = bpf_get_current_pid_tgid();
	__u32 pid = pid_tgid >> 32;
	__u32 tid = (__u32)pid_tgid;
	struct event event = {};

	if (target_pid && target_pid != pid)
		return 0;

	fill_event(&event, sock);
	event.pid = pid;
	event.backlog = backlog;
	bpf_map_update_elem(&values, &tid, &event, BPF_ANY);
	return 0;
}
```

### kproberet函数

函数定义：int BPF_KRETPROBE(函数名，参数)。example如下，宏会带上ctx，获取返回值需要ctx

```
SEC("kretprobe/dummy_file_write")
int BPF_KRETPROBE(file_write_exit, ssize_t ret)
{
	return probe_exit(ctx, WRITE, ret);
}
```

获取函数返回值：int ret = PT_REGS_RC(ctx);



## 资料

- [x86-64汇编入门 - nifengz](https://nifengz.com/introduction_x64_assembly/)
- [bcc/reference_guide.md at master · iovisor/bcc (github.com)](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)





