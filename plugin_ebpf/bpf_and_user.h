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

//------------------------ runqlatency
#define XM_RUNQLAT_MAX_SLOTS 20 // 2 ^ 20 = 1秒

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

struct syscall_event {
    pid_t pid;
    __u32 tid;
    __s64 syscall_nr;
    __s64 syscall_ret; // 调用返回值
    __u64 call_start_ns; // 调用开始时间
    __u64 call_delay_ns; // 调用耗时
    __u32 kernel_stack_id; // 调用堆栈
    __u32 user_stack_id;
};

//------------------------ cachestat_top
struct cachestat_top_statistics {
    __u64 add_to_page_cache_lru;
    __u64 ip_add_to_page_cache; // IP寄存器的值í
    __u64 mark_page_accessed;
    __u64 ip_mark_page_accessed; // IP寄存器的值
    __u64 account_page_dirtied;
    __u64 ip_account_page_dirtied; // IP寄存器的值
    __u64 mark_buffer_dirty;
    __u64 ip_mark_buffer_dirty; // IP寄存器的值
    __u32 uid; // 用户ID
    char comm[TASK_COMM_LEN]; // 进程名
};

#define CACHE_STATE_MAX_SIZE 1024