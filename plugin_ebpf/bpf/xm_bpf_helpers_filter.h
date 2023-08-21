/*
 * @Author: CALM.WU
 * @Date: 2023-08-16 14:38:45
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-16 15:14:45
 */
#pragma once

// false: 过滤掉
// true: 保留
static __always_inline bool filter_ts(void *arg_map, struct task_struct *ts,
                                      pid_t tgid) {
    __u32 index = 0;

    if (!ts) {
        return false;
    }

    // 通过map获取过滤参数
    struct xm_prog_filter_args *filter_args =
        bpf_map_lookup_elem(arg_map, &index);
    if (!filter_args) {
        // 如果没有设置，默认都过滤掉
        return false;
    }

    // bpf_printk("filter scope_type:%d, filter_cond_value:%lu",
    //            filter_args->scope_type, filter_args->filter_cond_value);

    if (filter_args->scope_type == XM_PROG_FILTER_TARGET_SCOPE_TYPE_OS) {
        // 全系统范围，都保留
        return true;
    } else if (filter_args->scope_type == XM_PROG_FILTER_TARGET_SCOPE_TYPE_NS) {
        // 判断namespace是否匹配
        __u32 ns_num = __xm_get_task_pidns(ts);
        if ((__u32)(filter_args->filter_cond_value) == ns_num) {
            return true;
        }
    } else if (filter_args->scope_type == XM_PROG_FILTER_TARGET_SCOPE_TYPE_CG) {
        // TODO: 判断cgroup，v1 or v2
        return true;
    } else if (filter_args->scope_type
               == XM_PROG_FILTER_TARGET_SCOPE_TYPE_PID) {
        // 判断pid
        if ((pid_t)(filter_args->filter_cond_value) == tgid) {
            return true;
        }
    } else if (filter_args->scope_type
               == XM_PROG_FILTER_TARGET_SCOPE_TYPE_PGID) {
        // 判断进程组
        pid_t pgid = __xm_get_task_pgid(ts);
        if ((pid_t)(filter_args->filter_cond_value) == pgid) {
            return true;
        }
    }
    return false;
}