/*
 * @Author: CALM.WU
 * @Date: 2022-02-15 14:06:36
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-10-24 14:52:25
 */

#pragma once

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

// Stack Traces are slightly different
// in that the value is 1 big byte array
// of the stack addresses
// max depth of each stack trace to track
#ifndef PERF_MAX_STACK_DEPTH
#define PERF_MAX_STACK_DEPTH 127
#define MAX_BIN_SEARCH_DEPTH 7
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
#define XM_RUNQLAT_MAX_SLOTS 20 // 2 ^ 20 = 1 秒

struct xm_runqlat_hist {
    __u32 slots[XM_RUNQLAT_MAX_SLOTS]; // 每个 slot 代表 2 的次方
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
    __u64 offcpu_duration_millsecs; // offcpu 时间，毫秒
    __u32 kernel_stack_id; // 调用堆栈
    __u32 user_stack_id;
    enum xm_cpu_sched_evt_type evt_type;
    pid_t tid; // 线程 id
    pid_t pid; // 进程 id
    char comm[TASK_COMM_LEN];
};

//------------------------ runqlen
#define XM_MAX_CPU_NR 128

//------------------------ trace_syscall
#define XM_MAX_FILTER_SYSCALL_COUNT 8 // 最多支持 8 个系统调用过滤

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
    __u64 ip_add_to_page_cache; // IP 寄存器的值
    __u64 mark_page_accessed;
    __u64 ip_mark_page_accessed; // IP 寄存器的值
    __u64 account_page_dirtied;
    __u64 ip_account_page_dirtied; // IP 寄存器的值
    __u64 mark_buffer_dirty;
    __u64 ip_mark_buffer_dirty; // IP 寄存器的值
    __u32 uid; // 用户 ID
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
    pid_t tid; // 线程 id
    pid_t pid; // 进程 id
    char comm[TASK_COMM_LEN];
    enum xm_processvm_evt_type evt_type;
    __u64 addr;
    __u64 len;
    //__u64 start_addr;
};

//------------------------ oom_kill
#define OOM_KILL_MSG_LEN 32
struct xm_oomkill_evt_data {
    pid_t tid; // 线程 id
    pid_t pid; // 进程 id
    __u32 memcg_id;
    __u64 memcg_page_counter; // physical memory page count
    __u64 points; // heuristic badness points
    __u64 rss_filepages;
    __u64 rss_anonpages;
    __u64 rss_shmepages;
    __u64 total_pages; // 全部的物理内存或者 cg 的配置的 limit 上限
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
        [XM_BIO_REQ_LATENCY_MAX_SLOTS]; // request
                                        // 在队列中等待的时间，如果是
                                        // direct，那么
                                        // insert=0，该值为
                                        // 0
    __u32 req_latency_slots[XM_BIO_REQ_LATENCY_MAX_SLOTS]; // 延迟，requet 从
                                                           // insert 或直接
                                                           // issue，到 complete
                                                           // 的延迟
    __u64 bytes; // 总共的字节数
    __u64 last_sector; // 该磁盘最后读取的扇区
    __u64 sequential_count; // 顺序操作次数
    __u64 random_count; // 随机操作次数
    __u64 req_err_count; // 失败次数
};

struct xm_bio_request_latency_evt_data {
    __u64 len; // bio 字节数
    __s64 req_in_queue_latency_us; // request 在队列中的延迟
    __s64 req_latency_us; // request 完成的延迟
    __s32 major; /* major number of driver */
    __s32 first_minor;
    pid_t tid; // 线程 id
    pid_t pid; // 进程 id
    __u8 cmd_flags; /* op and common flags */
    char comm[TASK_COMM_LEN];
};

//------------------------ profile
struct xm_task_userspace_regs {
    __u64 rip;
    __u64 rsp;
    __u64 rbp;
};

struct xm_profile_sample {
    pid_t pid; // 进程 id
    int kernel_stack_id; // 调用堆栈
    int user_stack_id;
    char comm[TASK_COMM_LEN];
};

#ifndef XM_FDE_TABLE_MAX_ROW_COUNT
#define XM_FDE_TABLE_MAX_ROW_COUNT 36
#endif

#ifndef XM_PATH_MAX_LEN
#define XM_PATH_MAX_LEN 128
#endif

#ifndef XM_PER_PROCESS_ASSOC_MODULE_COUNT
#define XM_PER_PROCESS_ASSOC_MODULE_COUNT 64
#endif

#ifndef XM_PER_MODULE_FDE_TABLE_COUNT
#define XM_PER_MODULE_FDE_TABLE_COUNT 2048
#endif

struct xm_profile_sample_data {
    __u32 count; // sample 数量
    __u32 pid_ns; // 归属的 pid namespace
    pid_t pgid; // 归属的进程组
};

struct xm_profile_dw_cfa {
    __s64 offset; // cfa 计算的偏移量
    __u64 reg; // cfa 计算的寄存器号，从该寄存器读取值，然后加上 offset
};

struct xm_profile_fde_row {
    __u64 loc;
    struct xm_profile_dw_cfa cfa;
    __s32 rbp_cfa_offset;
    __s32 ra_cfa_offset;
};

struct __attribute__((__packed__)) xm_profile_fde_table {
    __u64 start; // 起始地址
    __u64 end; // 结束地址
    struct xm_profile_fde_row rows[XM_FDE_TABLE_MAX_ROW_COUNT];
    __u32 row_count;
};

struct xm_profile_module_fde_tables {
    __u32 ref_count;
    __u32 fde_table_count;
    struct xm_profile_fde_table fde_tables[XM_PER_MODULE_FDE_TABLE_COUNT];
};

enum module_type {
    is_unknown = 0,
    is_exec,
    is_so,
};

struct xm_proc_maps_module {
    __u64 start_addr;
    __u64 end_addr;
    __u64 build_id_hash;
    enum module_type type; // so 需要 pc-start_addr
};

struct __attribute__((__packed__)) xm_pid_maps {
    struct xm_proc_maps_module modules[XM_PER_PROCESS_ASSOC_MODULE_COUNT];
    __u32 module_count;
};
