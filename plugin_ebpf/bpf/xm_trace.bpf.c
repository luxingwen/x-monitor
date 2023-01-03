/*
 * @Author: CALM.WU
 * @Date: 2022-12-26 15:12:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-26 17:22:50
 */

// 使用BPF_PROG_TYPE_BTF_RAW_TRACEPOINT来追踪进程的系统调用，对于btf_raw_tracepoint，使用BPF_PROG封装
// 1: 统计进程的系统调用次数
// 2: 统计进程的系统调用累积耗时
// 3: 统计进程的系统调用耗时分布
// 4: 过滤进程的系统调用

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "../bpf_and_user.h"

// 过滤系统调用的进程id
const volatile pid_t xm_trace_syscall_filter_pid = 0;

// 过滤系统调用的map
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, __s64); // syscall_nr
    __type(value, __s64); // 返回值
    __uint(max_entries, XM_MAX_FILTER_SYSCALL_COUNT);
} xm_filter_syscalls_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24); // 16M
} xm_syscalls_event_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, __u32); // tid
    __type(value, struct syscall_event); // 系统调用事件
    __uint(max_entries, 10000);
} xm_syscalls_record_map SEC(".maps");

// #define PERF_MAX_STACK_DEPTH		127
struct {
    __uint(type, BPF_MAP_TYPE_STACK_TRACE);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, PERF_MAX_STACK_DEPTH * sizeof(__u64));
    __uint(max_entries, 8196);
} xm_syscalls_stack_map SEC(".maps");

const struct syscall_event *unused __attribute__((unused));

// vmlinux.h typedef void (*btf_trace_sys_enter)(void *, struct pt_regs *, long
// int);

SEC("tp_btf/sys_enter")
__s32 BPF_PROG(xm_trace_rtp__sys_enter, struct pt_regs *regs,
               __s64 syscall_nr) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();
    struct syscall_event se;

    // 只过滤指定进程的系统调用
    if (!xm_trace_syscall_filter_pid
        || (xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid))
        return 0;

    // load program: permission denied: invalid indirect read from stack R3 off
    // -56+16 size 48 (67 line(s) omitted)
    // 对于栈变量必须初始化，不然map操作在虚拟机校验过程中会报错
    __builtin_memset(&se, 0, sizeof(struct syscall_event));
    se.call_start_ns = bpf_ktime_get_ns();
    se.syscall_nr = syscall_nr;
    se.pid = pid;
    se.tid = tid;

    bpf_map_update_elem(&xm_syscalls_record_map, &tid, &se, BPF_ANY);

    // if (syscall_nr == 267) {
    //     bpf_printk("xm_trace_syscalls pid:%d, tid:%d sys_enter
    //     sys_readlinkat",
    //                pid, tid);
    // }

    return 0;
}

// typedef void (*btf_trace_sys_exit)(void *, struct pt_regs *, long int);

SEC("tp_btf/sys_exit")
__s32 BPF_PROG(xm_trace_rtp__sys_exit, struct pt_regs *regs, __s64 ret) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();

    // 怎么从sys_exit中得到syscall_nr呢 %ax寄存器保存的是返回结果,
    // orig_ax保存的是系统调用号

    __s64 syscall_nr = BPF_CORE_READ(regs, orig_ax);

    if (!xm_trace_syscall_filter_pid
        || (xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid))
        return 0;

    struct syscall_event *se =
        bpf_map_lookup_elem(&xm_syscalls_record_map, &tid);
    if (se) {
        se->call_delay_ns = bpf_ktime_get_ns() - se->call_delay_ns;
        se->syscall_ret = ret;

        struct syscall_event *evt = bpf_ringbuf_reserve(
            &xm_syscalls_event_map, sizeof(struct syscall_event), 0);

        if (!evt) {
            bpf_printk("xm_trace_syscalls pid: %d, tid: %d, syscall_nr: %ld "
                       "reserve ringbuf  failed",
                       pid, tid, syscall_nr);
        } else {
            __builtin_memcpy(evt, se, sizeof(struct syscall_event));
            bpf_ringbuf_submit(evt, 0);
            bpf_map_delete_elem(&xm_syscalls_record_map, &tid);
        }
    }

    // if (syscall_nr == 267) {
    //     bpf_printk("xm_trace_syscalls pid:%d, tid:%d sys_exit
    //     sys_readlinkat",
    //                pid, tid);
    // }

    return 0;
}

#define XM_TRACE_KPROBE_PROG(name)                                        \
    __s32 xm_trace_kp__##name(struct pt_regs *ctx) {                      \
        pid_t pid = __xm_get_pid();                                       \
        __u32 tid = __xm_get_tid();                                       \
                                                                          \
        if (!xm_trace_syscall_filter_pid                                  \
            || (xm_trace_syscall_filter_pid                               \
                && pid != xm_trace_syscall_filter_pid))                   \
            return 0;                                                     \
                                                                          \
        struct syscall_event *se =                                        \
            bpf_map_lookup_elem(&xm_syscalls_record_map, &tid);           \
        if (se) {                                                         \
            __s32 stack_id = bpf_get_stackid(ctx, &xm_syscalls_stack_map, \
                                             KERN_STACKID_FLAGS);         \
            if (stack_id > 0) {                                           \
                se->stack_id = stack_id;                                  \
            }                                                             \
        }                                                                 \
        return 0;                                                         \
    }

// asmlinkage long sys_readlinkat(int dfd, const char __user *path, char __user
// *buf, int bufsiz);
SEC("kprobe/" SYSCALL(sys_readlinkat))
__s32 BPF_KPROBE(xm_trace_kp__sys_readlinkat, __s32 dfd, const char *path,
                 char *buf, int bufsiz) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();

    if (!xm_trace_syscall_filter_pid
        || (xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid))
        return 0;

    struct syscall_event *se =
        bpf_map_lookup_elem(&xm_syscalls_record_map, &tid);

    if (se) {
        __s32 stack_id =
            bpf_get_stackid(ctx, &xm_syscalls_stack_map, KERN_STACKID_FLAGS);
        if (stack_id > 0) {
            se->stack_id = stack_id;
        }
    }

    return 0;
}

SEC("kprobe/" SYSCALL(sys_openat))
__s32 BPF_KPROBE(xm_trace_kp__sys_openat, __s32 dfd, const char *filename,
                 __s32 flags, umode_t mode) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();

    if (!xm_trace_syscall_filter_pid
        || (xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid))
        return 0;

    struct syscall_event *se =
        bpf_map_lookup_elem(&xm_syscalls_record_map, &tid);

    if (se) {
        __s32 stack_id =
            bpf_get_stackid(ctx, &xm_syscalls_stack_map, KERN_STACKID_FLAGS);
        if (stack_id > 0) {
            se->stack_id = stack_id;
        }
    }

    return 0;
}

SEC("kprobe/" SYSCALL(sys_close))
XM_TRACE_KPROBE_PROG(sys_close)

char LICENSE[] SEC("license") = "GPL";
