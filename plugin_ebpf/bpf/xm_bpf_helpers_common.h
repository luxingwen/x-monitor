/*
 * @Author: CALM.WU
 * @Date: 2022-02-04 14:29:03
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 18:27:52
 */

// #include <linux/bpf.h> // uapi这个头文件和vmlinux.h不兼容啊，类型重复定义
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define TASK_COMM_LEN 16
#define MAX_THREAD_COUNT 10240
#define TASK_RUNNING 0

#ifndef memcpy
#define memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#endif

#define __stringify_1(x...) #x
#define __stringify(x...) __stringify_1(x)

#ifdef __x86_64__
#define SYSCALL(SYS) "__x64_" __stringify(SYS)
#elif defined(__s390x__)
#define SYSCALL(SYS) "__s390x_" __stringify(SYS)
#else
#define SYSCALL(SYS) __stringify(SYS)
#endif

#ifdef __TARGET_ARCH_x86
// KPROBE_REGS_IP_FIX这个宏是用来修正kprobe的上下文结构（struct
// pt_regs）中的指令指针（ip）的值的。因为在x86架构下，当kprobe被触发时，ip会指向被替换的原始指令，
// 而不是被探测的函数的地址。所以需要用KPROBE_REGS_IP_FIX宏来获取正确的函数地址，并存储在一个u64变量中。
#define KPROBE_REGS_IP_FIX(ip) (ip - sizeof(kprobe_opcode_t))
#else
#define KPROBE_REGS_IP_FIX(ip) ip
#endif

// max depth of each stack trace to track
#ifndef PERF_MAX_STACK_DEPTH
#define PERF_MAX_STACK_DEPTH 20
#endif

#ifndef PF_KTHREAD
#define PF_KTHREAD 0x00200000 /* I am a kernel thread */
#endif

// stack traces: the value is 1 big byte array of the stack addresses
typedef __u64 stack_trace_t[PERF_MAX_STACK_DEPTH];
#define BPF_STACK_TRACE(_name, _max_entries) \
    BPF_MAP(_name, BPF_MAP_TYPE_STACK_TRACE, u32, stack_trace_t, _max_entries)

#define KERN_STACKID_FLAGS (0 | BPF_F_FAST_STACK_CMP)
#define USER_STACKID_FLAGS (0 | BPF_F_FAST_STACK_CMP | BPF_F_USER_STACK)

#define GET_FIELD_ADDR(field) __builtin_preserve_access_index(&field)

#define READ_KERN(ptr)                                    \
    ({                                                    \
        typeof(ptr) _val;                                 \
        __builtin_memset((void *)&_val, 0, sizeof(_val)); \
        bpf_core_read((void *)&_val, sizeof(_val), &ptr); \
        _val;                                             \
    })

#define READ_KERN_STR_INTO(dst, src) \
    bpf_core_read_str((void *)&dst, sizeof(dst), src)

#define READ_USER(ptr)                                         \
    ({                                                         \
        typeof(ptr) _val;                                      \
        __builtin_memset((void *)&_val, 0, sizeof(_val));      \
        bpf_core_read_user((void *)&_val, sizeof(_val), &ptr); \
        _val;                                                  \
    })

#define BPF_READ(src, a, ...)                              \
    ({                                                     \
        ___type((src), a, ##__VA_ARGS__) __r;              \
        BPF_CORE_READ_INTO(&__r, (src), a, ##__VA_ARGS__); \
        __r;                                               \
    })

// bpf_probe_read_kernel(&exit_code, sizeof(exit_code), &task->exit_code);

#define PROCESS_EXIT_BPF_PROG(tpfn, hash_map)                                 \
    __s32 __##tpfn(void *ctx) {                                               \
        __u32 pid = __xm_get_pid();                                           \
        char comm[TASK_COMM_LEN];                                             \
        bpf_get_current_comm(&comm, sizeof(comm));                            \
                                                                              \
        __s32 ret = bpf_map_delete_elem(&hash_map, &pid);                     \
        if (0 == ret) {                                                       \
            struct task_struct *task =                                        \
                (struct task_struct *)bpf_get_current_task();                 \
            __s32 exit_code = BPF_CORE_READ(task, exit_code);                 \
                                                                              \
            bpf_printk("xm_ebpf_exporter pcomm: '%s' pid: %d exit_code: %d. " \
                       "remove element from hash_map.",                       \
                       comm, pid, exit_code);                                 \
        }                                                                     \
        return 0;                                                             \
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
static __always_inline __u32 __xm_get_tid() {
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
static __u32 __xm_get_task_pidns(struct task_struct *task) {
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

/* This function reads the cgroup id of a process. It's used by the
   kernel to get the cgroup id of a process. It's used in the
   get_task_cgroup_id() function to get the cgroup id of a process. */
static struct kernfs_node *
__xm_get_task_dflcgrp_assoc_kernfs(struct task_struct *task) {
    struct css_set *css;
    struct cgroup *dfl_cgrp;
    struct kernfs_node *kn;

    css = READ_KERN(task->cgroups);
    dfl_cgrp = READ_KERN(css->dfl_cgrp);
    kn = READ_KERN(dfl_cgrp->kn);
    return kn;
}

/* This function is used to get the cgroup id of the cgroup that a task is in.
 * The cgroup id is a unique identifier for a cgroup. The cgroup id is
 * assigned by the kernel when the cgroup is created. The cgroup id is
 * the same for all tasks that are in the same cgroup.
 *
 * task: The task whose cgroup id we want to get.
 * subsys_id: The id of the cgroup subsystem whose cgroup id we want to get.
 */
static __u64 __xm_get_task_cgid_by_subsys(struct task_struct *task,
                                          enum cgroup_subsys_id subsys_id) {
    struct css_set *css;
    struct cgroup_subsys_state *cg_ss;
    struct cgroup *cg;
    struct kernfs_node *kn;

    css = READ_KERN(task->cgroups);
    cg_ss = READ_KERN(css->subsys[subsys_id]);
    cg = READ_KERN(cg_ss->cgroup);
    kn = READ_KERN(cg->kn);
    return READ_KERN(kn->id);
}

// 获取进程的进程组ID，PIDTYPE_PGID
static pid_t __xm_get_task_pgid(struct task_struct *task) {
    struct signal_struct *signal;
    struct pid *pgid;
    struct upid session_upid;

    signal = READ_KERN(task->signal);
    pgid = READ_KERN(signal->pids[PIDTYPE_PGID]);
    session_upid = READ_KERN(pgid->numbers[0]);
    return session_upid.nr;
}

// 获取进程的session id，PIDTYPE_SID
static pid_t __xm_get_task_sessionid(struct task_struct *task) {
    struct signal_struct *signal;
    struct pid *session_pid;
    struct upid session_upid;

    signal = READ_KERN(task->signal);
    session_pid = READ_KERN(signal->pids[PIDTYPE_SID]);
    session_upid = READ_KERN(session_pid->numbers[0]);
    return session_upid.nr;
}

// This code checks if the task is a kernel thread or not. If it is a kernel
// thread, it will return true, otherwise it will return false
static __always_inline bool __xm_task_is_kthread(struct task_struct *task) {
    __u32 flags;
    flags = READ_KERN(task->flags);
    return flags & PF_KTHREAD;
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
static __s64 __xm_get_task_state(struct task_struct *task) {
    struct task_struct___new *t_new = (void *)task;

    if (bpf_core_field_exists(t_new->__state)) {
        return BPF_CORE_READ(t_new, __state);
    } else {
        /* 老内核里面field是task */
        struct task_struct___old *t_old = (void *)task;

        return BPF_CORE_READ(t_old, state);
    }
}
