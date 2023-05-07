/*
 * @Author: CALM.WU
 * @Date: 2022-02-15 14:06:36
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-27 15:08:03
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

//------------------------ runqlatency
#define XM_RUNQLAT_MAX_SLOTS 20 // 2 ^ 20 = 1秒

struct xm_runqlat_hist {
    __u32 slots[XM_RUNQLAT_MAX_SLOTS]; // 每个slot代表2的次方
};

//------------------------ cpu schedule event
enum xm_cpu_sched_evt_type {
    XM_CS_EVT_TYPE_NONE = 0,
    XM_CS_EVT_TYPE_RUNQLAT,
    XM_CS_EVT_TYPE_OFFCPU,
    XM_CS_EVT_TYPE_HANG,
};

// 结构体成员按字节大小从大到小排列
struct xm_cpu_sched_evt_data {
    __u64 offcpu_duration_millsecs; // offcpu时间，毫秒
    __u32 kernel_stack_id; // 调用堆栈
    __u32 user_stack_id;
    enum xm_cpu_sched_evt_type evt_type;
    pid_t pid; // 线程id
    pid_t tgid; // 进程id
    char comm[TASK_COMM_LEN];
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
    __u64 ip_add_to_page_cache; // IP寄存器的值
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

//------------------------ process vm monitor
enum xm_vmm_evt_type {
    XM_VMM_EVT_TYPE_NONE = 0,
    XM_VMM_EVT_TYPE_MMAP_ANON_PRIV,
    XM_VMM_EVT_TYPE_MMAP_SHARED,
    XM_VMM_EVT_TYPE_MMAP_OTHER,
    XM_VMM_EVT_TYPE_BRK,
};

// This is a structure definition for storing data related to virtual memory
// management events. It includes the process ID (pid), thread group ID (tgid),
// process name (comm), event type (evt_type), and length (len) of the event.
// The event type can be one of three types: none, mmap, or brk. This structure
// is likely used in a program that monitors and analyzes virtual memory
// management events in the system.
struct xm_vmm_evt_data {
    pid_t pid; // 线程id
    pid_t tgid; // 进程id
    char comm[TASK_COMM_LEN];
    enum xm_vmm_evt_type evt_type;
    __u64 len;
};
