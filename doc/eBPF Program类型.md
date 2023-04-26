# eBPF Program类型

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

文件libbpf.c中定义了SEC参数内容，**带有+号的意味前缀**。bpf_sec_def结构体定义了bpf_prog_type、bpf_attach_type类型变量

```
static const struct bpf_sec_def section_defs[] = {
	SEC_DEF("socket",		SOCKET_FILTER, 0, SEC_NONE),
	SEC_DEF("sk_reuseport/migrate",	SK_REUSEPORT, BPF_SK_REUSEPORT_SELECT_OR_MIGRATE, SEC_ATTACHABLE),
	SEC_DEF("sk_reuseport",		SK_REUSEPORT, BPF_SK_REUSEPORT_SELECT, SEC_ATTACHABLE),
	SEC_DEF("kprobe+",		KPROBE,	0, SEC_NONE, attach_kprobe),
	SEC_DEF("uprobe+",		KPROBE,	0, SEC_NONE, attach_uprobe),
	SEC_DEF("uprobe.s+",		KPROBE,	0, SEC_SLEEPABLE, attach_uprobe),
	SEC_DEF("kretprobe+",		KPROBE, 0, SEC_NONE, attach_kprobe),
	SEC_DEF("uretprobe+",		KPROBE, 0, SEC_NONE, attach_uprobe),
	SEC_DEF("uretprobe.s+",		KPROBE, 0, SEC_SLEEPABLE, attach_uprobe),
	SEC_DEF("kprobe.multi+",	KPROBE,	BPF_TRACE_KPROBE_MULTI, SEC_NONE, attach_kprobe_multi),
	SEC_DEF("kretprobe.multi+",	KPROBE,	BPF_TRACE_KPROBE_MULTI, SEC_NONE, attach_kprobe_multi),
	SEC_DEF("ksyscall+",		KPROBE,	0, SEC_NONE, attach_ksyscall),
	SEC_DEF("kretsyscall+",		KPROBE, 0, SEC_NONE, attach_ksyscall),
	SEC_DEF("usdt+",		KPROBE,	0, SEC_NONE, attach_usdt),
	SEC_DEF("tc",			SCHED_CLS, 0, SEC_NONE),
	SEC_DEF("classifier",		SCHED_CLS, 0, SEC_NONE),
	SEC_DEF("action",		SCHED_ACT, 0, SEC_NONE),
	SEC_DEF("tracepoint+",		TRACEPOINT, 0, SEC_NONE, attach_tp),
	SEC_DEF("tp+",			TRACEPOINT, 0, SEC_NONE, attach_tp),
	SEC_DEF("raw_tracepoint+",	RAW_TRACEPOINT, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("raw_tp+",		RAW_TRACEPOINT, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("raw_tracepoint.w+",	RAW_TRACEPOINT_WRITABLE, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("raw_tp.w+",		RAW_TRACEPOINT_WRITABLE, 0, SEC_NONE, attach_raw_tp),
	SEC_DEF("tp_btf+",		TRACING, BPF_TRACE_RAW_TP, SEC_ATTACH_BTF, attach_trace),
	SEC_DEF("fentry+",		TRACING, BPF_TRACE_FENTRY, SEC_ATTACH_BTF, attach_trace),
	SEC_DEF("fmod_ret+",		TRACING, BPF_MODIFY_RETURN, SEC_ATTACH_BTF, attach_trace),
	SEC_DEF("fexit+",		TRACING, BPF_TRACE_FEXIT, SEC_ATTACH_BTF, attach_trace),
	SEC_DEF("fentry.s+",		TRACING, BPF_TRACE_FENTRY, SEC_ATTACH_BTF | SEC_SLEEPABLE, attach_trace),
	SEC_DEF("fmod_ret.s+",		TRACING, BPF_MODIFY_RETURN, SEC_ATTACH_BTF | SEC_SLEEPABLE, attach_trace),
	SEC_DEF("fexit.s+",		TRACING, BPF_TRACE_FEXIT, SEC_ATTACH_BTF | SEC_SLEEPABLE, attach_trace),
	SEC_DEF("freplace+",		EXT, 0, SEC_ATTACH_BTF, attach_trace),
	SEC_DEF("lsm+",			LSM, BPF_LSM_MAC, SEC_ATTACH_BTF, attach_lsm),
	SEC_DEF("lsm.s+",		LSM, BPF_LSM_MAC, SEC_ATTACH_BTF | SEC_SLEEPABLE, attach_lsm),
	SEC_DEF("lsm_cgroup+",		LSM, BPF_LSM_CGROUP, SEC_ATTACH_BTF),
	SEC_DEF("iter+",		TRACING, BPF_TRACE_ITER, SEC_ATTACH_BTF, attach_iter),
	SEC_DEF("iter.s+",		TRACING, BPF_TRACE_ITER, SEC_ATTACH_BTF | SEC_SLEEPABLE, attach_iter),
	SEC_DEF("syscall",		SYSCALL, 0, SEC_SLEEPABLE),
	SEC_DEF("xdp.frags/devmap",	XDP, BPF_XDP_DEVMAP, SEC_XDP_FRAGS),
	SEC_DEF("xdp/devmap",		XDP, BPF_XDP_DEVMAP, SEC_ATTACHABLE),
	SEC_DEF("xdp.frags/cpumap",	XDP, BPF_XDP_CPUMAP, SEC_XDP_FRAGS),
	SEC_DEF("xdp/cpumap",		XDP, BPF_XDP_CPUMAP, SEC_ATTACHABLE),
	SEC_DEF("xdp.frags",		XDP, BPF_XDP, SEC_XDP_FRAGS),
	SEC_DEF("xdp",			XDP, BPF_XDP, SEC_ATTACHABLE_OPT),
	SEC_DEF("perf_event",		PERF_EVENT, 0, SEC_NONE),
	SEC_DEF("lwt_in",		LWT_IN, 0, SEC_NONE),
	SEC_DEF("lwt_out",		LWT_OUT, 0, SEC_NONE),
	SEC_DEF("lwt_xmit",		LWT_XMIT, 0, SEC_NONE),
	SEC_DEF("lwt_seg6local",	LWT_SEG6LOCAL, 0, SEC_NONE),
	SEC_DEF("sockops",		SOCK_OPS, BPF_CGROUP_SOCK_OPS, SEC_ATTACHABLE_OPT),
	SEC_DEF("sk_skb/stream_parser",	SK_SKB, BPF_SK_SKB_STREAM_PARSER, SEC_ATTACHABLE_OPT),
	SEC_DEF("sk_skb/stream_verdict",SK_SKB, BPF_SK_SKB_STREAM_VERDICT, SEC_ATTACHABLE_OPT),
	SEC_DEF("sk_skb",		SK_SKB, 0, SEC_NONE),
	SEC_DEF("sk_msg",		SK_MSG, BPF_SK_MSG_VERDICT, SEC_ATTACHABLE_OPT),
	SEC_DEF("lirc_mode2",		LIRC_MODE2, BPF_LIRC_MODE2, SEC_ATTACHABLE_OPT),
	SEC_DEF("flow_dissector",	FLOW_DISSECTOR, BPF_FLOW_DISSECTOR, SEC_ATTACHABLE_OPT),
	SEC_DEF("cgroup_skb/ingress",	CGROUP_SKB, BPF_CGROUP_INET_INGRESS, SEC_ATTACHABLE_OPT),
	SEC_DEF("cgroup_skb/egress",	CGROUP_SKB, BPF_CGROUP_INET_EGRESS, SEC_ATTACHABLE_OPT),
	SEC_DEF("cgroup/skb",		CGROUP_SKB, 0, SEC_NONE),
	SEC_DEF("cgroup/sock_create",	CGROUP_SOCK, BPF_CGROUP_INET_SOCK_CREATE, SEC_ATTACHABLE),
	SEC_DEF("cgroup/sock_release",	CGROUP_SOCK, BPF_CGROUP_INET_SOCK_RELEASE, SEC_ATTACHABLE),
	SEC_DEF("cgroup/sock",		CGROUP_SOCK, BPF_CGROUP_INET_SOCK_CREATE, SEC_ATTACHABLE_OPT),
	SEC_DEF("cgroup/post_bind4",	CGROUP_SOCK, BPF_CGROUP_INET4_POST_BIND, SEC_ATTACHABLE),
	SEC_DEF("cgroup/post_bind6",	CGROUP_SOCK, BPF_CGROUP_INET6_POST_BIND, SEC_ATTACHABLE),
	SEC_DEF("cgroup/bind4",		CGROUP_SOCK_ADDR, BPF_CGROUP_INET4_BIND, SEC_ATTACHABLE),
	SEC_DEF("cgroup/bind6",		CGROUP_SOCK_ADDR, BPF_CGROUP_INET6_BIND, SEC_ATTACHABLE),
	SEC_DEF("cgroup/connect4",	CGROUP_SOCK_ADDR, BPF_CGROUP_INET4_CONNECT, SEC_ATTACHABLE),
	SEC_DEF("cgroup/connect6",	CGROUP_SOCK_ADDR, BPF_CGROUP_INET6_CONNECT, SEC_ATTACHABLE),
	SEC_DEF("cgroup/sendmsg4",	CGROUP_SOCK_ADDR, BPF_CGROUP_UDP4_SENDMSG, SEC_ATTACHABLE),
	SEC_DEF("cgroup/sendmsg6",	CGROUP_SOCK_ADDR, BPF_CGROUP_UDP6_SENDMSG, SEC_ATTACHABLE),
	SEC_DEF("cgroup/recvmsg4",	CGROUP_SOCK_ADDR, BPF_CGROUP_UDP4_RECVMSG, SEC_ATTACHABLE),
	SEC_DEF("cgroup/recvmsg6",	CGROUP_SOCK_ADDR, BPF_CGROUP_UDP6_RECVMSG, SEC_ATTACHABLE),
	SEC_DEF("cgroup/getpeername4",	CGROUP_SOCK_ADDR, BPF_CGROUP_INET4_GETPEERNAME, SEC_ATTACHABLE),
	SEC_DEF("cgroup/getpeername6",	CGROUP_SOCK_ADDR, BPF_CGROUP_INET6_GETPEERNAME, SEC_ATTACHABLE),
	SEC_DEF("cgroup/getsockname4",	CGROUP_SOCK_ADDR, BPF_CGROUP_INET4_GETSOCKNAME, SEC_ATTACHABLE),
	SEC_DEF("cgroup/getsockname6",	CGROUP_SOCK_ADDR, BPF_CGROUP_INET6_GETSOCKNAME, SEC_ATTACHABLE),
	SEC_DEF("cgroup/sysctl",	CGROUP_SYSCTL, BPF_CGROUP_SYSCTL, SEC_ATTACHABLE),
	SEC_DEF("cgroup/getsockopt",	CGROUP_SOCKOPT, BPF_CGROUP_GETSOCKOPT, SEC_ATTACHABLE),
	SEC_DEF("cgroup/setsockopt",	CGROUP_SOCKOPT, BPF_CGROUP_SETSOCKOPT, SEC_ATTACHABLE),
	SEC_DEF("cgroup/dev",		CGROUP_DEVICE, BPF_CGROUP_DEVICE, SEC_ATTACHABLE_OPT),
	SEC_DEF("struct_ops+",		STRUCT_OPS, 0, SEC_NONE),
	SEC_DEF("sk_lookup",		SK_LOOKUP, BPF_SK_LOOKUP, SEC_ATTACHABLE),
};
```

SEC的匹配逻辑，+号的处理，实际上内核不同这里的实现稍有不同，新内核逻辑会有扩充

```
static bool sec_def_matches(const struct bpf_sec_def *sec_def, const char *sec_name)
{
	size_t len = strlen(sec_def->sec);

	/* "type/" always has to have proper SEC("type/extras") form */
	if (sec_def->sec[len - 1] == '/') {
		if (str_has_pfx(sec_name, sec_def->sec))
			return true;
		return false;
	}

	/* "type+" means it can be either exact SEC("type") or
	 * well-formed SEC("type/extras") with proper '/' separator
	 */
	if (sec_def->sec[len - 1] == '+') {
		len--;
		/* not even a prefix */
		if (strncmp(sec_name, sec_def->sec, len) != 0)
			return false;
		/* exact match or has '/' separator */
		if (sec_name[len] == '\0' || sec_name[len] == '/')
			return true;
		return false;
	}

	return strcmp(sec_name, sec_def->sec) == 0;
}
```



## Program的函数编写

编写最多的是kprobe、tracepoint、**btf raw tracepoint**的函数，为了简便函数定义和参数读取，下面分类描述。

### Program Context参数

所有的eBPF program都接受一个context指针作为参数，但它所指向的结构取决于触发事件的类型，eBPF程序员需要编写接受适当类型上下文的程序。

### Tracepoint  Program

#### enter函数

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

- example-1:

  ```
  SEC("tp/syscalls/sys_enter_openat")
  int handle_openat_enter(struct trace_event_raw_sys_enter *ctx)
  ```

- example-2：

  函数定义，获取参数，args[1]是char **argv，

  ```
  SEC("tracepoint/syscalls/sys_enter_execve")
  int tracepoint__syscalls__sys_enter_execve(struct trace_event_raw_sys_enter* ctx)
  {
  	u64 id;
  	pid_t pid, tgid;
  	unsigned int ret;
  	struct event *event;
  	struct task_struct *task;
  	const char **args = (const char **)(ctx->args[1]);
  	const char *argp;
  ```

  execve的tracepoint参数定义

  ```
  ➜  ~ bpftrace -lv tracepoint:syscalls:sys_enter_execve
  tracepoint:syscalls:sys_enter_execve
      int __syscall_nr
      const char * filename
      const char *const * argv
      const char *const * envp
  ```

  execve函数定义

  ```
         int execve(const char *filename, char *const argv[],
                    char *const envp[])
  ```


#### exit函数

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

### KProbe program 

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
  
  也可以直接使用struct pt_regs *ctx作为参数，需要使用PT_REGS_XXX宏来读取参数信息，第一个参数是从1开始的，而不是0。
  
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

### KProbeRet program

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

### raw_tracepoint program

可以通过查看`/sys/kernel/debug/tracing/available_events` 文件的内容找到 **raw_tracepoint 可监控**的事件。raw_tracepoint相对于tracepoint不会将函数参数展开，而是读取的原始参数

- 函数定义，SEC格式：SEC("raw_tracepoint/<name>")或SEC("raw_tp/<name>")，参数类型struct bpf_raw_tracepoint_args *ctx，

  ```
  SEC("raw_tracepoint/cgroup_mkdir")
  int tracepoint__cgroup__cgroup_mkdir(struct bpf_raw_tracepoint_args *ctx)
  {
      event_data_t data = {};
      if (!init_event_data(&data, ctx))
          return 0;
  
      struct cgroup *dst_cgrp = (struct cgroup *) ctx->args[0];
      char *path = (char *) ctx->args[1];
  ```

- 参数定义，需要在内核代码中查找，而不是在format文件中

  ```
  DEFINE_EVENT(cgroup, cgroup_rmdir,
  
  	TP_PROTO(struct cgroup *cgrp, const char *path),
  
  	TP_ARGS(cgrp, path)
  );
  ```

  在内核代码中宏DEFINE_EVENT、TRACE_EVENT、TRACE_EVENT_FN定义能找到参数定义TP_PROTO、TP_ARGS。

### tp_btf/fentry/fexit program

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

  - 确定参数，所有btf_raw_tracepoint函数在vmlinux中都存在一个名为btf_trace_<name>函数指针定义，以上面函数为例。**参数确定比raw tracepoing函数方便**

    ```
     calmwu@localhost  ~/program/cpp_space/x-monitor/plugin_ebpf/bpf/.output  grep btf_trace vmlinux.h 
    	bool attach_btf_trace;
    typedef void (*btf_trace_initcall_level)(void *, const char *);
    typedef void (*btf_trace_initcall_start)(void *, initcall_t);
    ```
    
    可以在内核源码中找到tracepoint函数定义
    
    ```
    TRACE_EVENT(block_rq_complete,
    	TP_PROTO(struct request *rq, int error, unsigned int nr_bytes),
    	TP_ARGS(rq, error, nr_bytes),
    ```

- fentry

- fexit

### perf_event program

```
SEC("perf_event")
int do_sample(struct bpf_perf_event_data *ctx)
```

什么时候运行？取决于perf事件触发和所选择的采样率，这些采样率由perf事件属性结构中的freq和sample_period字段指定。每个采样间隔执行一次。

### fmod_ret

原理，如果fmod_ret函数返回非0，不会调用原生函数。可以用来拦截系统调用

```
Here is an example of how a fmod_ret program behaves:

int func_to_be_attached(int a, int b)
{  <--- do_fentry

do_fmod_ret:
   <update ret by calling fmod_ret>
   if (ret != 0)
        goto do_fexit;

original_function:

    <side_effects_happen_here>

}  <--- do_fexit

ALLOW_ERROR_INJECTION(func_to_be_attached, ERRNO)

The fmod_ret program attached to this function can be defined as:

SEC("fmod_ret/func_to_be_attached")
BPF_PROG(func_name, int a, int b, int ret)
{
        // This will skip the original function logic.
        return -1;
}
```

### socket-related program types - SOCKET_FILTER, SK_SKB, SOCK_OPS

### tc (traffic control) subsystem programs

### xdp : the Xpress Data Path

## 资料

- [x86-64汇编入门 - nifengz](https://nifengz.com/introduction_x64_assembly/)
- [bcc/reference_guide.md at master · iovisor/bcc (github.com)](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)
- [ebpf/libbpf 程序使用 btf raw tracepoint 的常见问题 - mozillazg's Blog](https://mozillazg.com/2022/06/ebpf-libbpf-btf-powered-enabled-raw-tracepoint-common-questions.html#hidsec)
- [Introduce BPF_MODIFY_RET tracing progs. [LWN.net\]](https://lwn.net/Articles/813724/)
- [c++ - eBPF: raw_tracepoint arguments - Stack Overflow](https://stackoverflow.com/questions/70652825/ebpf-raw-tracepoint-arguments)
- [Writing asm for eBPF programs with the Go eBPF library (fntlnz.wtf)](https://fntlnz.wtf/post/bpf-go-asm/)
- [Introducing gobpf - Using eBPF from Go | Kinvolk](https://kinvolk.io/blog/2016/11/introducing-gobpf-using-ebpf-from-go/)
- [BPF: A Tour of Program Types (oracle.com)](https://blogs.oracle.com/linux/post/bpf-a-tour-of-program-types)
- [BPF In Depth: BPF Helper Functions (oracle.com)](https://blogs.oracle.com/linux/post/bpf-in-depth-bpf-helper-functions)





