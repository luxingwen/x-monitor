/*
 * @Author: CALM.WU
 * @Date: 2023-08-16 15:15:40
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-16 17:18:49
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_math.h"

#include "../bpf_and_user.h"
#include "xm_bpf_helpers_filter.h"

// prog参数，过滤条件
BPF_ARRAY(xm_profile_arg_map, struct xm_prog_filter_args, 1);
// 堆栈计数
BPF_HASH(xm_profile_sample_count_map, struct xm_profile_sample,
         struct xm_profile_sample_data, MAX_THREAD_COUNT);
struct {
    __uint(type, BPF_MAP_TYPE_STACK_TRACE);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, PERF_MAX_STACK_DEPTH * sizeof(__u64));
    __uint(max_entries, 10240);
} xm_profile_stack_map SEC(".maps");

const enum xm_prog_filter_target_scope_type __unused_filter_scope_type
    __attribute__((unused)) = XM_PROG_FILTER_TARGET_SCOPE_TYPE_NONE;

const struct xm_profile_sample *__unused_ps __attribute__((unused));
const struct xm_profile_sample_data *__unused_psd __attribute__((unused));

SEC("perf_event")
__s32 xm_do_perf_event(struct bpf_perf_event_data *ctx) {
    pid_t tid;
    struct xm_profile_sample ps = {
        .pid = 0,
        .kernel_stack_id = -1,
        .user_stack_id = -1,
        .comm[0] = '\0',
    };
    struct xm_profile_sample_data init_psd = {
        .count = 1,
        .pid_ns = 0,
        .pgid = 0,
    }, *val;

    // 过滤条件判断
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    // 读取线程id
    BPF_CORE_READ_INTO(&tid, ts, pid);
    if (tid == 0) {
        return 0;
    }
    // 读取进程id，如果id=0，表明是内核线程
    BPF_CORE_READ_INTO(&ps.pid, ts, tgid);

    if (ps.pid == 0) {
        bpf_printk("xm_do_perf_event this kernel task_struct.tgid:%d, tid:%d",
                   ps.pid, tid);
    }

    if (!filter_ts(&xm_profile_arg_map, ts, ps.pid)) {
        return 0;
    }

    BPF_CORE_READ_INTO(&ps.pid, ts, tgid);
    // 获取comm
    BPF_CORE_READ_STR_INTO(&ps.comm, ts, group_leader, comm);
    //  获取内核、用户堆栈
    ps.kernel_stack_id =
        bpf_get_stackid(ctx, &xm_profile_stack_map, KERN_STACKID_FLAGS);
    ps.user_stack_id =
        bpf_get_stackid(ctx, &xm_profile_stack_map, USER_STACKID_FLAGS);

    init_psd.pid_ns = __xm_get_task_pidns(ts);
    init_psd.pgid = __xm_get_task_pgid(ts);

    val = __xm_bpf_map_lookup_or_try_init((void *)&xm_profile_sample_count_map,
                                          &ps, &init_psd);
    if (val) {
        // 进程堆栈计数递增
        __sync_fetch_and_add(&(val->count), 1);
    }

    // bpf_printk("xm_do_perf_event pid:%d, kernel_stack_id:%d, "
    //            "user_stack_id:%d",
    //            ps.pid, ps.kernel_stack_id, ps.user_stack_id);
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";