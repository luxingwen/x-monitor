/*
 * @Author: CALM.WU
 * @Date: 2022-02-04 14:29:03
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-10-20 11:23:28
 */

//#include <linux/bpf.h> // uapi这个头文件和vmlinux.h不兼容啊，类型重复定义
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define TASK_COMM_LEN 16

#ifndef memcpy
#define memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#endif

#define __stringify_1(x...) #x
#define __stringify(x...) __stringify_1(x)

#ifndef PERF_MAX_STACK_DEPTH
#define PERF_MAX_STACK_DEPTH 127
#endif

// bpf_probe_read_kernel(&exit_code, sizeof(exit_code), &task->exit_code);

#define PROCESS_EXIT_BPF_PROG(tpfn, hash_map)                                \
    __s32 __##tpfn(void *ctx) {                                              \
        __u32 pid = __xm_get_pid();                                          \
        char comm[TASK_COMM_LEN];                                            \
        bpf_get_current_comm(&comm, sizeof(comm));                           \
                                                                             \
        __s32 ret = bpf_map_delete_elem(&hash_map, &pid);                    \
        if (0 == ret) {                                                      \
            struct task_struct *task =                                       \
                (struct task_struct *)bpf_get_current_task();                \
            __s32 exit_code = BPF_CORE_READ(task, exit_code);                \
                                                                             \
            bpf_printk("xmonitor pcomm: '%s' pid: %d exit_code: %d. remove " \
                       "element from hash_map.",                             \
                       comm, pid, exit_code);                                \
        }                                                                    \
        return 0;                                                            \
    }

static __always_inline void __xm_update_u64(__u64 *res, __u64 value) {
    __sync_fetch_and_add(res, value);
    if ((0xFFFFFFFFFFFFFFFF - *res) <= value) {
        *res = value;
    }
}

// 进程id
static __always_inline __u32 __xm_get_pid() {
    return bpf_get_current_pid_tgid() >> 32;
}

// 线程id
static __always_inline __s32 __xm_get_tid() {
    return bpf_get_current_pid_tgid() & 0xFFFFFFFF;
}

/**
 * Returns the parent process ID of the given task.
 *
 * @param task The task whose parent process ID is to be returned.
 *
 * @returns The parent process ID of the given task.
 */
static __always_inline __s32 __xm_get_ppid(struct task_struct *task) {
    __u32 ppid;
    struct task_struct *parent = NULL;
    BPF_CORE_READ_INTO(&parent, task, real_parent);
    BPF_CORE_READ_INTO(&ppid, parent, tgid);
    return ppid;
}

/**
 * Returns the start time of the process.
 *
 * @param task The task struct for the process.
 *
 * @returns The start time of the process.
 */
static __always_inline __u64
__xm_get_process_start_time(struct task_struct *task) {
    __u64 start_time = 0;
    BPF_CORE_READ_INTO(&start_time, task, start_time);
    return start_time;
}

/**
 * Returns the PID namespace of the current thread.
 * https://www.dubaojiang.com/category/linux/linux_kernel/process/  局部 ID
 * 与命名空间
 */
static __always_inline __u32 __xm_get_pid_namespace(struct task_struct *task) {
    struct pid *thread_pid = NULL;
    __u32 level;
    struct upid upid;
    __u32 ns_num = 0;

    // thread_pid = task->thread_pid
    BPF_CORE_READ_INTO(&thread_pid, task, thread_pid);
    // level = thread_pid->level
    BPF_CORE_READ_INTO(&level, thread_pid, level);
    //
    BPF_CORE_READ_INTO(&upid, thread_pid, numbers[level]);
    // bpf_core_read(&upid, sizeof(upid), &thread_pid->numbers[level]);
    // 如果是对象，必须写在一起，用.分割，如果是指针，必须用->分割
    BPF_CORE_READ_INTO(&ns_num, upid.ns, ns.inum);
    return ns_num;
}

/* new kernel task_struct definition */
struct task_struct___new {
    long __state;
} __attribute__((preserve_access_index));

/* old kernel task_struct definition */
struct task_struct___old {
    long state;
} __attribute__((preserve_access_index));

/**
 * Gets the state of a task.
 *
 * @param task The task whose state is to be retrieved.
 *
 * @returns The state of the task.
 */
static __always_inline __s64 __xm_get_task_state(struct task_struct *task) {
    struct task_struct___new *t_new = (void *)task;

    if (bpf_core_field_exists(t_new->__state)) {
        return BPF_CORE_READ(t_new, __state);
    } else {
        /* 老内核里面field是task */
        struct task_struct___old *t_old = (void *)task;

        return BPF_CORE_READ(t_old, state);
    }
}
