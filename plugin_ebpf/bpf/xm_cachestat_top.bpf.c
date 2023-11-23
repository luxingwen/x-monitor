/*
 * @Author: calmwu
 * @Date: 2022-02-19 21:48:44
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 15:01:13
 */

#include <vmlinux.h>
#include "../bpf_and_user.h"
#include "xm_bpf_helpers_common.h"

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, CACHE_STATE_MAX_SIZE);
    __type(key, __u32);
    __type(value, struct cachestat_top_statistics);
} xm_cachestat_top_map SEC(".maps");

#if 0
SEC("kprobe/do_unlinkat")
int BPF_KPROBE(do_unlinkat, int dfd, struct filename *name) {
    pid_t pid;
    const char *filename;

    pid = bpf_get_current_pid_tgid() >> 32;
    filename = BPF_CORE_READ(name, name);
    bpf_printk("KPROBE ENTRY pid = %d, filename = %s\n", pid, filename);
    return 0;
}
#endif

SEC("kprobe/add_to_page_cache_lru")
int BPF_KPROBE(kprobe__xm_add_to_page_cache_lru) {
    __s32 ret = 0;
    __u32 pid;
    struct cachestat_top_statistics *fill;

    // 得到进程 ID
    pid = __xm_get_pid();

    fill = bpf_map_lookup_elem(&xm_cachestat_top_map, &pid);
    if (fill) {
        __xm_update_u64(&fill->add_to_page_cache_lru, 1);
        // 有可能因为 execve 导致 comm 改变
        bpf_get_current_comm(&fill->comm, sizeof(fill->comm));
        bpf_printk("kprobe__xm_add_to_page_cache_lru pcomm: '%s' pid: %d "
                   "value: %lu",
                   fill->comm, pid, fill->add_to_page_cache_lru);

    } else {
        struct cachestat_top_statistics init_value = {
            .add_to_page_cache_lru = 1,
        };
        init_value.ip_add_to_page_cache = PT_REGS_IP(ctx);
        init_value.uid = (uid_t)bpf_get_current_uid_gid();
        // 得到应用程序名
        bpf_get_current_comm(&init_value.comm, sizeof(init_value.comm));

        ret = bpf_map_update_elem(&xm_cachestat_top_map, &pid, &init_value,
                                  BPF_NOEXIST);
        if (0 == ret) {
            bpf_printk("xmonitor add_to_page_cache_lru add new pcomm: '%s' "
                       "pid: %d successed",
                       init_value.comm, pid);
        }
    }
    return 0;
}

SEC("kprobe/mark_page_accessed")
int BPF_KPROBE(kprobe__xm_mark_page_accessed) {
    __s32 ret = 0;
    __u32 pid;
    struct cachestat_top_statistics *fill;

    // 得到进程 ID
    pid = __xm_get_pid();

    fill = bpf_map_lookup_elem(&xm_cachestat_top_map, &pid);
    if (fill) {
        __xm_update_u64(&fill->mark_page_accessed, 1);
        // 有可能因为 execve 导致 comm 改变
        bpf_get_current_comm(&fill->comm, sizeof(fill->comm));
        bpf_printk("kprobe__xm_mark_page_accessed comm: '%s' pid: %d "
                   "value: %lu",
                   fill->comm, pid, fill->mark_page_accessed);

    } else {
        struct cachestat_top_statistics init_value = {
            .mark_page_accessed = 1,
        };
        init_value.ip_mark_page_accessed = PT_REGS_IP(ctx);
        init_value.uid = (uid_t)bpf_get_current_uid_gid();
        // 得到应用程序名
        bpf_get_current_comm(&init_value.comm, sizeof(init_value.comm));

        ret = bpf_map_update_elem(&xm_cachestat_top_map, &pid, &init_value,
                                  BPF_NOEXIST);
        if (0 == ret) {
            bpf_printk("kprobe__xm_mark_page_accessed add new pcomm: '%s' pid: "
                       "%d successed",
                       init_value.comm, pid);
        }
    }
    return 0;
}

SEC("kprobe/account_page_dirtied")
int BPF_KPROBE(kprobe__xm_account_page_dirtied) {
    __s32 ret = 0;
    __u32 pid;
    struct cachestat_top_statistics *fill;

    // 得到进程 ID
    pid = __xm_get_pid();

    fill = bpf_map_lookup_elem(&xm_cachestat_top_map, &pid);
    if (fill) {
        __xm_update_u64(&fill->account_page_dirtied, 1);
        bpf_get_current_comm(&fill->comm, sizeof(fill->comm));
        bpf_printk("kprobe__xm_account_page_dirtied comm: '%s' pid: %d "
                   "value: %lu",
                   fill->comm, pid, fill->account_page_dirtied);

    } else {
        struct cachestat_top_statistics init_value = {
            .account_page_dirtied = 1,
        };
        init_value.ip_account_page_dirtied = PT_REGS_IP(ctx);
        init_value.uid = (uid_t)bpf_get_current_uid_gid();
        // 得到应用程序名
        bpf_get_current_comm(&init_value.comm, sizeof(init_value.comm));

        ret = bpf_map_update_elem(&xm_cachestat_top_map, &pid, &init_value,
                                  BPF_NOEXIST);
        if (0 == ret) {
            bpf_printk("kprobe__xm_account_page_dirtied add new comm: '%s' "
                       "pid: %d successed",
                       init_value.comm, pid);
        }
    }
    return 0;
}

SEC("kprobe/mark_buffer_dirty")
int BPF_KPROBE(kprobe__xm_mark_buffer_dirty) {
    __s32 ret = 0;
    __u32 pid;
    struct cachestat_top_statistics *fill;

    // 得到进程 ID
    pid = __xm_get_pid();

    fill = bpf_map_lookup_elem(&xm_cachestat_top_map, &pid);
    if (fill) {
        __xm_update_u64(&fill->mark_buffer_dirty, 1);
        // 有可能因为 execve 导致 comm 改变
        bpf_get_current_comm(&fill->comm, sizeof(fill->comm));
        bpf_printk("kprobe__xm_mark_buffer_dirty comm: '%s' pid: %d "
                   "value: %lu",
                   fill->comm, pid, fill->mark_buffer_dirty);
        // bpf_map_update_elem(&xm_cachestat_top_map, &pid, fill, BPF_ANY);
    } else {
        struct cachestat_top_statistics init_value = {
            .mark_buffer_dirty = 1,
        };
        init_value.ip_mark_buffer_dirty = PT_REGS_IP(ctx);
        init_value.uid = (uid_t)bpf_get_current_uid_gid();
        // 得到应用程序名
        bpf_get_current_comm(&init_value.comm, sizeof(init_value.comm));

        ret = bpf_map_update_elem(&xm_cachestat_top_map, &pid, &init_value,
                                  BPF_NOEXIST);
        if (0 == ret) {
            bpf_printk("kprobe__xm_mark_buffer_dirty comm: '%s' pid: %d "
                       "successed",
                       init_value.comm, pid);
        }
    }
    return 0;
}
#if 0
// #define PROG(tpfn)                                                                                \
//     __s32 __##tpfn(void *ctx)                                                                     \
//     {                                                                                             \
//         __s32 pid = __xm_get_pid();                                                           \
//         char  comm[TASK_COMM_LEN];                                                                \
//         bpf_get_current_comm(&comm, sizeof(comm));                                                \
//                                                                                                   \
//         __s32 ret = bpf_map_delete_elem(&xm_cachestat_top_map, &pid);                                    \
//         if (0 == ret) {                                                                           \
//             struct task_struct *task =                                                            \
//                 (struct task_struct *)bpf_get_current_task();                                     \
//             __s32 exit_code;                                                                      \
//             bpf_probe_read_kernel(&exit_code, sizeof(exit_code),                                  \
//                                   &task->exit_code);                                              \
//                                                                                                   \
//             bpf_printk(                                                                               \
//                 "xmonitor pcomm: '%s' pid: %d exit_code: %d. remove element from xm_cachestat_top_map.", \
//                 comm, pid, exit_code);                                                            \
//         }                                                                                         \
//         return 0;                                                                                 \
//     }

// SEC("tracepoint/sched/sched_process_exit")
// PROCESS_EXIT_BPF_PROG(xm_cst_sched_process_exit, xm_cachestat_top_map)

// SEC("tracepoint/sched/sched_process_free")
// PROCESS_EXIT_BPF_PROG(xm_cst_sched_process_free, xm_cachestat_top_map)
#endif

static void wno_unused_headerfuncs() {
    /* don't need to actually call the functions to avoid the warnings */
    (void)&__xm_get_task_pidns;
    (void)&__xm_get_task_state;
    return;
}

char _license[] SEC("license") = "GPL";