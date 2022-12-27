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
    __type(key, u32); // syscall_nr
    __type(value, u32); // 返回值
    __uint(max_entries, XM_MAX_FILTER_SYSCALL_COUNT);
} xm_filter_syscalls_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, pid_t);
    __type(value, struct xm_trace_syscall_datarec);
    __uint(max_entries, 1);
} xm_trace_syscalls_datarec_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, XM_MAX_SYSCALL_NR);
    __type(key, u32); // tid
    __type(value, __u64); // 系统调用起始时间
} xm_trace_syscalls_start_time_map;

// vmlinux.h typedef void (*btf_trace_sys_enter)(void *, struct pt_regs *, long
// int);

SEC("tp_btf/sys_enter")
__s32 BPF_PROG(xm_btf_rtp__sys_enter, struct pt_regs *regs, __s64 syscall_nr) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();

    // 只过滤指定进程的系统调用
    if (!xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid)
        return 0;

    __u64 call_start_ns = bpf_ktime_get_ns();
    bpf_map_update_elem(&xm_trace_syscalls_start_time_map, &tid, &call_start_ns,
                        BPF_ANY);

    bpf_printk("xm trace_syscalls enter: pid=%d, syscall_nr=%ld", pid,
               syscall_nr);

    return 0;
}

// typedef void (*btf_trace_sys_exit)(void *, struct pt_regs *, long int);

SEC("tp_btf/sys_exit")
__s32 BPF_PROG(xm_btf_rtp__sys_exit, struct pt_regs *regs, __s32 ret) {
    pid_t pid = __xm_get_pid();
    __u32 tid = __xm_get_tid();
    __u64 delay_ns = 0;
    static struct xm_trace_syscall_datarec zero = { 0 };

    // 怎么从sys_exit中得到syscall_nr呢
    __s64 syscall_nr = BPF_CORE_READ(regs, r8); // regs->r8;

    if (!xm_trace_syscall_filter_pid && pid != xm_trace_syscall_filter_pid)
        return 0;

    __u64 *call_start_ns =
        bpf_map_lookup_elem(&xm_trace_syscalls_start_time_map, &tid);
    if (call_start_ns) {
        delay_ns = bpf_ktime_get_ns() - *call_start_ns;
    }

    struct xm_trace_syscall_datarec *val = __xm_bpf_map_lookup_or_try_init(
        &xm_trace_syscalls_datarec_map, &pid, &zero);
    if (val) {
        __xm_update_u64(&val->syscall_total_count, 1);
        __xm_update_u64(&val->syscall_total_ns, delay_ns);

        if (delay_ns > val->syscall_slowest_ns) {
            val->syscall_slowest_ns = delay_ns;
            val->syscall_slowest_nr = syscall_nr;
        }

        if (ret < 0) {
            __xm_update_u64(&val->syscall_failed_count, 1);
            val->syscall_failed_nr = syscall_nr;
            val->syscall_errno = ret;
        }

        bpf_printk("xm trace_syscalls exit: pid: %d, syscall_nr: %ld, ret: %d, "
                   "delay_ns: %lu",
                   pid, syscall_nr, ret, delay_ns);
    }
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
