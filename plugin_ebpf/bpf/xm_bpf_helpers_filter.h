/*
 * @Author: CALM.WU
 * @Date: 2023-08-16 14:38:45
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-16 15:14:45
 */
#pragma once

static __always_inline bool is_kthread() {
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    if (!task) {
        return false;
    }

    void *mm;
    int err = bpf_probe_read_kernel(&mm, 8, &task->mm);
    if (err) {
        return false;
    }

    return mm == NULL;
}

// false: 过滤掉
// true: 保留
static __always_inline bool filter_ts(void *arg_map, struct task_struct *ts,
                                      pid_t tgid) {
    __u32 index = 0;

    if (!ts) {
        return false;
    }

    // 通过 map 获取过滤参数
    struct xm_prog_filter_args *filter_args =
        bpf_map_lookup_elem(arg_map, &index);
    if (!filter_args) {
        // 如果没有设置，默认都过滤掉
        return false;
    }

    // bpf_printk("filter scope_type:%d, filter_cond_value:%lu",
    //            filter_args->scope_type, filter_args->filter_cond_value);

    if (filter_args->scope_type == XM_PROG_FILTER_TARGET_SCOPE_TYPE_OS) {
        return true;
    } else if (filter_args->scope_type == XM_PROG_FILTER_TARGET_SCOPE_TYPE_NS) {
        // 判断 namespace 是否匹配
        __u32 ns_num = __xm_get_task_pidns(ts);
        if ((__u32)(filter_args->filter_cond_value) == ns_num) {
            return true;
        }
    } else if (filter_args->scope_type == XM_PROG_FILTER_TARGET_SCOPE_TYPE_CG) {
        // TODO: 判断 cgroup，v1 or v2
        return true;
    } else if (filter_args->scope_type
               == XM_PROG_FILTER_TARGET_SCOPE_TYPE_PID) {
        // 判断 pid
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

/**
 * Check if the current task is in the host mount namespace.
 *
 * @return 1 if the current task is in the host mount namespace, 0 otherwise.
 */
static __always_inline __s32 __xm_is_host_mntns() {
    struct task_struct *current_task;
    struct nsproxy *nsproxy;
    struct mnt_namespace *mnt_ns;
    __u32 inum;

    current_task = (struct task_struct *)bpf_get_current_task();

    BPF_CORE_READ_INTO(&nsproxy, current_task, nsproxy);
    BPF_CORE_READ_INTO(&mnt_ns, nsproxy, mnt_ns);
    BPF_CORE_READ_INTO(&inum, mnt_ns, ns.inum);

    if (inum == 0xF0000000) {
        return 1;
    }
    return 0;
}

/**
 * @brief Check if the current process is running inside a container.
 *
 * This function checks if the current process is running inside a container
 * by calling the __xm_is_host_mntns() function and negating its result.
 *
 * @return 1 if the current process is running inside a container, 0 otherwise.
 */
static __always_inline __s32 __xm_is_container() {
    return !__xm_is_host_mntns();
}
