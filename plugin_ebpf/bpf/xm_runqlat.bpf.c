/*
 * @Author: calmwu
 * @Date: 2022-02-28 14:24:40
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-02 16:02:28
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_net.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_math.h"

#include "../bpf_and_user.h"

#define MAX_THREAD_COUNT 10240
#define TASK_RUNNING 0

// 从task状态设置为TASK_RUNNING插入运行队列，到被调度上CPU，统计等待时间
// 检查系统中CPU调度器延迟（运行队列的延迟，rbtree）。在需求超过供给，CPU资源处于饱和状态时，runqlat统计的信息是每个线程等待CPU的耗时
// 这里使用btf raw tracepoint，其和常规的raw tracepoint有一定的差别
// https://mozillazg.com/2022/06/ebpf-libbpf-btf-powered-enabled-raw-tracepoint-common-questions.html#hidsec

// const volatile struct xm_runqlat_args runqlat_args = { .filter_type =
//                                                            FILTER_DEF_OS,
//                                                        .id = 0 };

// 1: os，2：namespace，3：CGroup，4：PID，5：PGID
const volatile __s32 gather_obj_type = 1;
const volatile __s64 gather_obj_val = 0;

// 只支持一个cgroup
// struct {
//     __uint(type, BPF_MAP_TYPE_CGROUP_ARRAY);
//     __type(key, __u32);
//     __type(value, __u32);
//     __uint(max_entries, 1);
// } xm_runqlat_cgroup_map SEC(".maps");

// 记录每个thread wakeup的时间
// struct {
//     __uint(type, BPF_MAP_TYPE_HASH);
//     __uint(max_entries, MAX_THREAD_COUNT);
//     __type(key, __u32);
//     __type(value, __u64);
// } xm_runqlat_start_map SEC(".maps");
BPF_HASH(xm_runqlat_start_map, __u32, __u64, MAX_THREAD_COUNT);
// 存放数据统计直方图，thread被sechedule后，xm_runqlat_hist.slot[x]+1
// struct {
//     __uint(type, BPF_MAP_TYPE_HASH);
//     __uint(max_entries, 1);
//     __type(key, __u32);
//     __type(value, struct xm_runqlat_hist);
// } xm_runqlat_hists_map SEC(".maps");
BPF_HASH(xm_runqlat_hists_map, __u32, struct xm_runqlat_hist, 1);

static struct xm_runqlat_hist __zero_hist = {
    .slots = { 0 },
};

static __always_inline __s32 __trace_enqueue(pid_t tgid, pid_t pid) {
    // if (runqlat_args.filter_type == FILTER_SPEC_CGROUP) {
    //     // 如果是指定cgroup，判断该ts是否在cgroup中
    //     if (!bpf_current_task_under_cgroup(&xm_runqlat_cgroup_map, 0)) {
    //         return 0;
    //     }
    // } else if (runqlat_args.filter_type == FILTER_SPEC_PROCESS) {
    //     if ((pid_t)runqlat_args.id != tgid) {
    //         return 0;
    //     }
    // }

    // 得到wakeup时间，单位纳秒
    __u64 wakeup_ns = bpf_ktime_get_ns();
    // 更新map，记录thread wakeup time
    bpf_map_update_elem(&xm_runqlat_start_map, &pid, &wakeup_ns, BPF_ANY);
    return 0;
}

// // /sys/kernel/debug/tracing/events/sched/sched_wakeup/format
// SEC("raw_tracepoint/sched_wakeup")
// __s32 xm_rql_raw_tp__sw(struct bpf_raw_tracepoint_args *ctx) {
//     pid_t tgid, pid;

//     struct task_struct *p = (struct task_struct *)ctx->args[0];
//     BPF_CORE_READ_INTO(&tgid, p, tgid);
//     BPF_CORE_READ_INTO(&pid, p, pid);
//     bpf_printk("xm_ebpf_exporter runqlat sched_wakeup tgid:%d, pid:%d", tgid,
//                pid);
//     __trace_enqueue(tgid, pid);
//     return 0;
// }

// SEC("raw_tracepoint/sched_wakeup_new")
// __s32 xm_rql_raw_tp__swn(struct bpf_raw_tracepoint_args *ctx) {
//     pid_t tgid, pid;

//     struct task_struct *p = (struct task_struct *)ctx->args[0];
//     BPF_CORE_READ_INTO(&tgid, p, tgid);
//     BPF_CORE_READ_INTO(&pid, p, pid);
//     bpf_printk("xm_ebpf_exporter runqlat sched_wakeup_new tgid:%d, pid:%d",
//                tgid, pid);
//     __trace_enqueue(tgid, pid);
//     return 0;
// }

// ts被唤醒
SEC("tp_btf/sched_wakeup")
__s32 BPF_PROG(xm_btp_sched_wakeup, struct task_struct *ts) {
    // bpf_printk("xm_ebpf_exporter runqlat sched_wakeup pid:%d", ts->pid);
    // tgid：进程号，pid：线程号
    return __trace_enqueue(ts->tgid, ts->pid);
}

SEC("tp_btf/sched_wakeup_new")
__s32 BPF_PROG(xm_btp_sched_wakeup_new, struct task_struct *ts) {
    // ts被唤醒
    // bpf_printk("xm_ebpf_exporter runqlat sched_wakeup_new pid:%d", ts->pid);
    return __trace_enqueue(ts->tgid, ts->pid);
}

SEC("tp_btf/sched_switch")
__s32 BPF_PROG(xm_btp_sched_switch, bool preempt, struct task_struct *prev,
               struct task_struct *next) {
    struct xm_runqlat_hist *hist = NULL;

    // 调度出cpu的task，如果状态还是RUNNING，继续插入wakeup
    // map，算成一种重新唤醒
    if (__xm_get_task_state(prev) == TASK_RUNNING) {
        __trace_enqueue(prev->tgid, prev->pid);
    }

    // 因为是raw BTF tracepoint，所以可以直接使用内核结构
    // next是被选中，即将上cpu的ts
    pid_t pid = next->pid;
    if (pid == 0) {
        // pid = 0的情况：
        // Summary:
        // 1.idle is a process with a PID of 0.
        // 2. Idle on the main processor evolved from the original process
        // (pid=0). Idle from the processor is obtained by the Init process
        // fork, but their PID is 0.
        // The 3.Idle process is the lowest priority.
        // And does not participate in scheduling. It is only dispatched when
        // the execution queue is empty.
        // The 4.Idle loop waits for the
        // need_resched to be placed. Use HLT to save energy by default.
        // char zero_comm[TASK_COMM_LEN] = { 0 };
        // READ_KERN_STR_INTO(zero_comm, next->comm);
        // bpf_trace_printk: xm_ebpf_exporter runqlat, zero pid comm:''
        // bpf_printk("xm_ebpf_exporter runqlat, zero pid comm:'%s'",
        // zero_comm);
        return 0;
    }

    __u64 *wakeup_ns = bpf_map_lookup_elem(&xm_runqlat_start_map, &pid);
    if (!wakeup_ns) {
        // wakeup 时间不存在
        bpf_printk("xm_ebpf_exporter runqlat pid:%d is not in "
                   "xm_runqlat_start_map.",
                   pid);
        return 0;
    }

    // 通过kernfs_node得到id
    //__u64 cgroup_id = __xm_get_task_memcg_id(next); // READ_KERN(kn->id);
    // __u64 cgroup_id = __xm_get_task_cgid_by_subsys(next, memory_cgrp_id);
    // pid_t pgid = __xm_get_task_pgid(next);
    // pid_t session_id = __xm_get_task_sessionid(next);
    // __u32 pidns_id = __xm_get_task_pidns(next);
    // bpf_printk("xm_ebpf_exporter runqlat sched_switch pid:%d, pidns_id:%u",
    // pid,
    //            pidns_id);
    // bpf_printk("\tmemcg_id:%d, pgid:%d, sessionid:%d", cgroup_id, pgid,
    //            session_id);

    // ** 如何去读取char*成员
    // 得到task对应的default cgroup绑定的kernfs_node
    // struct kernfs_node *kn = __xm_get_task_dflcgrp_assoc_kernfs(next);
    // const char *kernfs_name = READ_KERN(kn->name);
    // char cgroup_name[32] = { 0 };
    // __s64 ret = READ_KERN_STR_INTO(cgroup_name, kernfs_name);
    // // bpf_probe_read_kernel_str(&cgroup_name, sizeof(cgroup_name),
    // // kernfs_name);
    // if (ret == 1) {
    //     /*
    //     int task_cgroup_path(struct task_struct *task, char *buf, size_t
    //     buflen)
    //     */
    //     cgroup_name[0] = '/';
    //     cgroup_name[1] = 0;
    // }
    // bpf_printk("xm_ebpf_exporter runqlat sched_switch cgroup_name:'%s', "
    //            "ret:%ld",
    //            cgroup_name, ret);

    // 进程从被唤醒到on cpu的时间
    __s64 wakeup_to_run_duration = bpf_ktime_get_ns() - *wakeup_ns;
    if (wakeup_to_run_duration < 0)
        goto cleanup;

    __u32 hkey = -1;
    // if (runqlat_args.filter_type == FILTER_PER_THREAD) {
    //     hkey = next->pid;
    // } else if (runqlat_args.filter_type == FILTER_PER_PROCESS) {
    //     hkey = next->tgid;
    // } else if (runqlat_args.filter_type == FILTER_PER_PIDNS) {
    //     hkey = (__s32)__xm_get_task_pidns(next);
    // }

    hist = (struct xm_runqlat_hist *)__xm_bpf_map_lookup_or_try_init(
        &xm_runqlat_hists_map, &hkey, &__zero_hist);
    if (!hist) {
        bpf_printk("xm_ebpf_exporter runqlat lookup or init "
                   "xm_runqlat_hists_map failed.");
        goto cleanup;
    }

    // 单位是微秒
    wakeup_to_run_duration = wakeup_to_run_duration / 1000U;
    // 计算2的对数，求出在哪个slot, 总共26个槽位，1---2的25次方
    // !! I found out the reason, is because of slot definition. "u32 slot"
    // !! doesn't work, u64 works, this is very strange.
    __u64 slot = __xm_log2l(wakeup_to_run_duration);
    // 超过最大槽位，放到最后一个槽位
    if (slot >= XM_RUNQLAT_MAX_SLOTS) {
        slot = XM_RUNQLAT_MAX_SLOTS - 1;
    }
    __sync_fetch_and_add(&hist->slots[slot], 1);

cleanup:
    // 获得cpu的ts，从map中删除
    bpf_map_delete_elem(&xm_runqlat_start_map, &pid);

    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
