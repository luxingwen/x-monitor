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

- example:

  ```
  SEC("tp/syscalls/sys_enter_openat")
  int handle_openat_enter(struct trace_event_raw_sys_enter *ctx)
  ```

### tracepoint exit函数

- 参数：struct trace_event_raw_sys_exit *ctx

  读取返回值

  ```
  unsigned int fd = (unsigned int)ctx->ret;
  ```

- example：

  ```
  SEC("tp/syscalls/sys_exit_openat")
  int handle_openat_exit(struct trace_event_raw_sys_exit *ctx)
  ```

### kprobe函数

- 函数定义：int BPF_KPROBE(函数名, 内核函数参数)，BPF_KPROBE宏实际是展开了struct pt_regs *ctx（Registers and BPF context）。

  example：

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
  
  也可以直接使用struct pt_regs *ctx作为参数，需要使用PT_REGS_XXX宏来读取参数信息
  
  ```
  SEC("kprobe/__seccomp_filter")
  int bpf_prog1(struct pt_regs *ctx)
  {
  	int sc_nr = (int)PT_REGS_PARM1(ctx);
  ```
  
  如果参数是结构体
  
  ```
  PROG(SYS__NR_write)(struct pt_regs *ctx)
  {
  	struct seccomp_data sd;
  
  	bpf_probe_read_kernel(&sd, sizeof(sd), (void *)PT_REGS_PARM2(ctx));
  ```

### kproberet函数

- 函数定义：int BPF_KRETPROBE(函数名，参数)。example如下，宏会带上ctx，获取返回值需要ctx。

  example：

  ```
  SEC("kretprobe/dummy_file_write")
  int BPF_KRETPROBE(file_write_exit, ssize_t ret)
  {
  	return probe_exit(ctx, WRITE, ret);
  }
  ```

- 获取函数返回值：int ret = PT_REGS_RC(ctx);

### tp_btf/fentry/fexit函数

- btf raw tracepoint

  tp_btf是编写BPF_TRACE_RAW_TP程序的section前缀。

  ```
  SEC_DEF("tp_btf/", TRACING, .expected_attach_type = BPF_TRACE_RAW_TP,
  		.is_attach_btf = true, .attach_fn = attach_trace),
  ```

  btf raw tracepoint 指的是 [BTF-powered raw tracepoint (tp_btf) 或者说是 BTF-enabled raw tracepoint](https://lore.kernel.org/netdev/20201203204634.1325171-1-andrii@kernel.org/t/) ，更常规的raw tracepoint相比有个最重要的区别：btf版本可以直接在ebpf程序中访问内核内存，不需要像常规raw tracepoint一样需要借助bpf_core_read或bpf_probe_read_kernel这样的辅助函数才能访问内核内存。-------**好像很少这样做**。

  - 函数定义，SEC("tp_btf/<name>")，name和普通的tracepoint函数没什么区别

    ```
    SEC("tp_btf/block_rq_complete")
    int BPF_PROG(block_rq_complete, struct request *rq, int error,
    	     unsigned int nr_bytes)
    ```

    宏BPF_PROG会自动展开ctx对应到tracepoint函数参数。也可以如下定义。

    ```
    SEC("tp_btf/block_rq_complete")
    int btf_raw_tracepoint__block_rq_complete(u64 *ctx)
    ```

    其中 `ctx[0]` 对应上面 `btf_trace_block_rq_complete` 中 `void *` 后面的第一个参数 `struct pt_regs *`, `ctx[1]` 是第二个参数 `struct request *` 。这两个参数的含义跟前面 [raw tracepoint](https://mozillazg.com/2022/05/ebpf-libbpf-raw-tracepoint-common-questions.html) 中所说的 `TP_PROTO(struct request *rq, int error, unsigned int nr_bytes)` 中的含义是一样的。

  - 确定参数，所有事件在vmlinux中都存在一个名为btf_trace_<name>函数指针定义，以上面函数为例。**参数确定比raw tracepoing函数方便**

    ```
    typedef void (*btf_trace_block_rq_complete)(void *, struct request *, int, unsigned int);
    ```

    可以在内核源码中找到tracepoint函数定义

    ```
    TRACE_EVENT(block_rq_complete,
    	TP_PROTO(struct request *rq, int error, unsigned int nr_bytes),
    	TP_ARGS(rq, error, nr_bytes),
    ```

- fentry

- fexit

### perf_event函数

```
SEC("perf_event")
int do_sample(struct bpf_perf_event_data *ctx)
```

触发执行，每个采样间隔执行一次。

## 资料

- [x86-64汇编入门 - nifengz](https://nifengz.com/introduction_x64_assembly/)
- [bcc/reference_guide.md at master · iovisor/bcc (github.com)](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)
- [ebpf/libbpf 程序使用 btf raw tracepoint 的常见问题 - mozillazg's Blog](https://mozillazg.com/2022/06/ebpf-libbpf-btf-powered-enabled-raw-tracepoint-common-questions.html#hidsec)





