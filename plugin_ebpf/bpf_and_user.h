/*
 * @Author: CALM.WU
 * @Date: 2022-02-15 14:06:36
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-16 16:13:57
 */

#pragma once

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

#define XDP_UNKNOWN XDP_REDIRECT + 1
#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_UNKNOWN + 1)
#endif

//
enum xm_prog_filter_target_scope_type {
    XM_PROG_FILTER_TARGET_SCOPE_TYPE_NONE = 0,
    XM_PROG_FILTER_TARGET_SCOPE_TYPE_OS,
    XM_PROG_FILTER_TARGET_SCOPE_TYPE_NS,
    XM_PROG_FILTER_TARGET_SCOPE_TYPE_CG,
    XM_PROG_FILTER_TARGET_SCOPE_TYPE_PID,
    XM_PROG_FILTER_TARGET_SCOPE_TYPE_PGID,
    XM_PROG_FILTER_TARGET_SCOPE_TYPE_MAX,
};

struct xm_prog_filter_args {
    enum xm_prog_filter_target_scope_type scope_type;
    __u64 filter_cond_value;
};

//------------------------ xdp_stats_datarec

struct xdp_stats_datarec {
    __u64 rx_packets;
    __u64 rx_bytes;
};

// struct xm_ebpf_event {
//     pid_t pid;
//     pid_t ppid;
//     char comm[TASK_COMM_LEN];
//     __s32 err_code; // 0 means success, otherwise means failed
// };

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
    XM_CS_EVT_TYPE_PROCESS_EXIT,
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
enum xm_processvm_evt_type {
    XM_PROCESSVM_EVT_TYPE_NONE = 0,
    XM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV,
    XM_PROCESSVM_EVT_TYPE_MMAP_SHARED,
    XM_PROCESSVM_EVT_TYPE_BRK,
    XM_PROCESSVM_EVT_TYPE_BRK_SHRINK,
    XM_PROCESSVM_EVT_TYPE_MUNMAP,
    XM_PROCESSVM_EVT_TYPE_MAX,
};

struct xm_processvm_evt_data {
    pid_t tid; // 线程id
    pid_t pid; // 进程id
    char comm[TASK_COMM_LEN];
    enum xm_processvm_evt_type evt_type;
    __u64 addr;
    __u64 len;
    //__u64 start_addr;
};

//------------------------ oom_kill
#define OOM_KILL_MSG_LEN 32
struct xm_oomkill_evt_data {
    pid_t tid; // 线程id
    pid_t pid; // 进程id
    __u32 memcg_id;
    __u64 memcg_page_counter; // physical memory page count
    __u64 points; // heuristic badness points
    __u64 rss_filepages;
    __u64 rss_anonpages;
    __u64 rss_shmepages;
    __u64 total_pages; // 全部的物理内存或者cg的配置的limit上限
    char comm[TASK_COMM_LEN];
    char msg[OOM_KILL_MSG_LEN];
};

//------------------------ bio

struct xm_bio_key {
    __s32 major; /* major number of driver */
    __s32 first_minor;
    __u8 cmd_flags; /* op and common flags */
};

#define XM_BIO_REQ_LATENCY_MAX_SLOTS 20

struct xm_bio_data {
    __u32 req_in_queue_latency_slots
        [XM_BIO_REQ_LATENCY_MAX_SLOTS]; // request在队列中等待的时间，如果是direct，那么insert=0，该值为0
    __u32 req_latency_slots
        [XM_BIO_REQ_LATENCY_MAX_SLOTS]; // 延迟，requet从insert或直接issue，到complete的延迟
    __u64 bytes; // 总共的字节数
    __u64 last_sector; // 该磁盘最后读取的扇区
    __u64 sequential_count; // 顺序操作次数
    __u64 random_count; // 随机操作次数
    __u64 req_err_count; // 失败次数
};

struct xm_bio_request_latency_evt_data {
    __u64 len; // bio字节数
    __s64 req_in_queue_latency_us; // request在队列中的延迟
    __s64 req_latency_us; // request完成的延迟
    __s32 major; /* major number of driver */
    __s32 first_minor;
    pid_t tid; // 线程id
    pid_t pid; // 进程id
    __u8 cmd_flags; /* op and common flags */
    char comm[TASK_COMM_LEN];
};

//------------------------ profile
struct xm_profile_sample {
    pid_t pid; // 进程id
    __u32 kernel_stack_id; // 调用堆栈
    __u32 user_stack_id;
    char comm[TASK_COMM_LEN];
};