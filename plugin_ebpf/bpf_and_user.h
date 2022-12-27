/*
 * @Author: CALM.WU
 * @Date: 2022-02-15 14:06:36
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-13 15:08:45
 */

#pragma once

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

#define XDP_UNKNOWN XDP_REDIRECT + 1
#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_UNKNOWN + 1)
#endif

struct xdp_stats_datarec {
    __u64 rx_packets;
    __u64 rx_bytes;
};

struct xm_ebpf_event {
    pid_t pid;
    pid_t ppid;
    char comm[TASK_COMM_LEN];
    __s32 err_code; // 0 means success, otherwise means failed
};

enum task_filter_scope_type {
    FILTER_DEF_OS = 0, // 整个系统按线程统计，整个系统是一个直方图
    FILTER_PER_THREAD, // 按线程过滤 task_struct.pid，每个线程是一个直方图
    FILTER_PER_PROCESS, // 按进程过滤 task_struct.tgid，每个进程使一个直方图
    FILTER_PER_PIDNS, // 按pidns过滤，每个pidns是个直方图
    FILTER_SPEC_CGROUP, // 只统计指定的cgroup下的thread
    FILTER_SPEC_PROCESS, // 只统计指定进程下的thread
};

//------------------------ runqlat
#define XM_RUNQLAT_MAX_SLOTS 26

struct xm_runqlat_args {
    enum task_filter_scope_type filter_type;
    __u64 id;
};

struct xm_runqlat_hist {
    __u32 slots[XM_RUNQLAT_MAX_SLOTS]; // 每个slot代表2的次方
};

//------------------------ runqlen
#define XM_MAX_CPU_NR 128

//------------------------ trace_syscall
#define XM_MAX_FILTER_SYSCALL_COUNT 8 // 最多支持8个系统调用过滤
#define XM_MAX_SYSCALL_NR \
    512 // 最多支持1024个系统调用, 该文件可查看最大系统调用号
        // /usr/include/asm/unistd_64.h
struct xm_trace_syscall_datarec {
    __u64 syscall_total_count; // 系统调用总次数
    __u64 syscall_failed_count; // 系统调用失败次数
    __u64 syscall_total_ns; // 系统调用总耗时, 纳秒
    __u64 syscall_slowest_ns; // 系统调用最慢耗时, 纳秒
    __u64 syscall_slowest_nr; // 系统调用最慢耗时的系统调用号
    __u64 syscall_failed_nr; // 系统调用失败的系统调用号
    __s32 syscall_errno; // 系统调用失败的错误码
};