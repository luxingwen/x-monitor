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

// 过滤系统调用的进程id
const volatile pid_t xm_trace_syscall_filter_pid = 0;
// 是否统计系统调用耗时
const volatile bool xm_trace_syscall_time_cost = false;

// vmlinux.h typedef void (*btf_trace_sys_enter)(void *, struct pt_regs *, long
// int);

SEC("tp_btf/sys_enter")
__s32 BPF_PROG(xm_btf_rtp__sys_enter, struct pt_regs *regs, __s64 syscall_nr) {
    return 0;
}

// typedef void (*btf_trace_sys_exit)(void *, struct pt_regs *, long int);

SEC("tp_btf/sys_exit")
__s32 BPF_PROG(xm_btf_rtp__sys_exit, struct pt_regs *regs, __s64 ret) {
    // 怎么从sys_exit中得到syscall_nr呢
    __s64 syscall_nr = BPF_CORE_READ(regs, r8); // regs->r8;
    return 0;
}
