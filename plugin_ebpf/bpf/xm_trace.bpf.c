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
    __uint(max_entries, XM_MAX_SYSCALL_NR);
    __type(key, __u32); // tid
    __type(value, __u64); // 系统调用起始时间
} xm_syscalls_start_time_map SEC(".maps");

// struct {
//     __uint(type, BPF_MAP_TYPE_HASH);
//     __uint(max_entries, XM_MAX_SYSCALL_NR);
//     __type(key, __u32); // tid
//     __type(value, __u32); // 系统调用堆栈id
// } xm_syscalls_stackid_map SEC(".maps");

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
__s32 BPF_PROG(xm_btf_rtp__sys_enter, struct pt_regs *regs, __s64 syscall_nr) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();

    // 只过滤指定进程的系统调用
    if (!xm_trace_syscall_filter_pid
        || (xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid))
        return 0;

    __u64 call_start_ns = bpf_ktime_get_ns();
    // __s32 stack_id =
    //     bpf_get_stackid(ctx, &xm_syscalls_stack_map, KERN_STACKID_FLAGS);

    bpf_map_update_elem(&xm_syscalls_start_time_map, &tid, &call_start_ns,
                        BPF_ANY);
    // if (stack_id >= 0) {
    //     bpf_map_update_elem(&xm_syscalls_stackid_map, &tid, (__u32
    //     *)&stack_id,
    //                         BPF_ANY);
    // }

    return 0;
}

// typedef void (*btf_trace_sys_exit)(void *, struct pt_regs *, long int);

SEC("tp_btf/sys_exit")
__s32 BPF_PROG(xm_btf_rtp__sys_exit, struct pt_regs *regs, __s64 ret) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();
    __u64 delay_ns = 0;

    // 怎么从sys_exit中得到syscall_nr呢 %ax寄存器保存的是返回结果,
    // orig_ax保存的是系统调用号

    __s64 syscall_nr = BPF_CORE_READ(regs, orig_ax);

    if (!xm_trace_syscall_filter_pid
        || (xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid))
        return 0;

    __u64 *call_start_ns =
        bpf_map_lookup_elem(&xm_syscalls_start_time_map, &tid);
    if (call_start_ns) {
        delay_ns = bpf_ktime_get_ns() - *call_start_ns;
    }

    struct syscall_event *evt = bpf_ringbuf_reserve(
        &xm_syscalls_event_map, sizeof(struct syscall_event), 0);
    if (!evt) {
        bpf_printk("xm_trace_syscalls pid: %d, tid: %d, syscall_nr: %ld lost",
                   pid, tid, syscall_nr);
    } else {
        evt->pid = pid;
        evt->tid = tid;
        evt->syscall_nr = syscall_nr;
        evt->syscall_ret = ret;
        evt->delay_ns = delay_ns;

        //__u32 *stack_id = bpf_map_lookup_elem(&xm_syscalls_stackid_map, &tid);
        __s32 stack_id =
            bpf_get_stackid(ctx, &xm_syscalls_stack_map, USER_STACKID_FLAGS);
        if (stack_id >= 0) {
            evt->stack_id = stack_id;
            bpf_printk("xm_trace_syscalls stack_id: %d", evt->stack_id);
        } else {
            bpf_printk("xm_trace_syscalls failed to get syscall_nr: %ld "
                       "stack_id",
                       syscall_nr);
            evt->stack_id = 0;
        }
        bpf_ringbuf_submit(evt, 0);
    }

    return 0;
}

char LICENSE[] SEC("license") = "GPL";
