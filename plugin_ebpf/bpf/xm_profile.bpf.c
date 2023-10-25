/*
 * @Author: CALM.WU
 * @Date: 2023-08-16 15:15:40
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-16 17:18:49
 */

#include <vmlinux.h>
#include "../bpf_and_user.h"
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_math.h"
#include "xm_bpf_helpers_filter.h"

// prog参数，过滤条件
BPF_ARRAY(xm_profile_arg_map, struct xm_prog_filter_args, 1);
// 堆栈计数
BPF_HASH(xm_profile_sample_count_map, struct xm_profile_sample,
         struct xm_profile_sample_data, MAX_THREAD_COUNT);
// 堆栈
BPF_STACK_TRACE(xm_profile_stack_map, 1024);
// 保存pid的内存映射信息，如果pid存在，说明需要执行来获取用户态堆栈
BPF_HASH(xm_profile_pid_modules_map, __u32, struct xm_pid_maps, 256);

// 保存每个module的fde table信息，内核对hash_map的value有大小限制
// if ((u64)attr->key_size + attr->value_size >= KMALLOC_MAX_SIZE -
//    sizeof(struct htab_elem))
// 	 if key_size + value_size is bigger, the user space won't be
// 	 * able to access the elements via bpf syscall. This check
// 	 * also makes sure that the elem_size doesn't overflow and it's
// 	 * kmalloc-able later in htab_map_update_elem()
//
// 	return -E2BIG;
BPF_HASH(xm_profile_module_fdetable_map, __u64,
         struct xm_profile_module_fde_tables, 256);

const enum xm_prog_filter_target_scope_type __unused_filter_scope_type
    __attribute__((unused)) = XM_PROG_FILTER_TARGET_SCOPE_TYPE_NONE;

const struct xm_profile_sample *__unused_ps __attribute__((unused));
const struct xm_profile_sample_data *__unused_psd __attribute__((unused));
const struct xm_profile_dw_rule *__unused_pdr __attribute__((unused));
const struct xm_profile_fde_row *__unused_pfr __attribute__((unused));
const struct xm_profile_fde_table *__unused_pft __attribute__((unused));
const struct xm_proc_maps_module *__unused_pmm __attribute__((unused));
const struct xm_pid_maps *__unused_pm __attribute__((unused));
const struct xm_profile_module_fde_tables *__unused_pmft
    __attribute__((unused));

// 获取用户task_struct用户空间的寄存器，为了使用eh_frame的信息做stack unwind
static __always_inline void
__xm_get_task_userspace_regs(struct task_struct *task,
                             struct pt_regs *user_regs,
                             struct xm_task_userspace_regs *tu_regs) {
    // 获取指令寄存器的值，判断了是否是内核地址空间
    if (in_kernel_space(PT_REGS_IP(user_regs))) {
        // !!由于系统调用在进程切换到内核地址空间，需要按ptrace的方式，来获取保存在stack中的用户态寄存器
        __u64 __ptr;
        __ptr = (__u64)READ_KERN(task->stack);
        __ptr += THREAD_SIZE - TOP_OF_KERNEL_STACK_PADDING;
        struct pt_regs *regs = ((struct pt_regs *)__ptr) - 1;
        tu_regs->rip = PT_REGS_IP_CORE(regs);
        tu_regs->rsp = PT_REGS_SP_CORE(regs);
        tu_regs->rbp = PT_REGS_FP_CORE(regs);
    } else {
        // perf event
        // snapshot当前task_struct在用户地址空间，直接从cur_regs中获取
        tu_regs->rip = PT_REGS_IP(user_regs);
        tu_regs->rsp = PT_REGS_SP(user_regs);
        tu_regs->rbp = PT_REGS_FP(user_regs);
    }
}

// int element = sortedArray[index];
static __always_inline __s32 __bsearch_fde_table(
    const struct xm_profile_fde_row *rows, __u32 row_count, __u64 pc) {
    if (!rows || 0 == row_count || row_count > PERF_MAX_STACK_DEPTH) {
        return -1;
    }

    __s32 left = 0;
    __s32 right = row_count - 1;
    __s32 result = -1;
    __s32 mid = 0;
    // 堆栈深度是127，二分查找最多7次
    for (__s32 i = 0; i < MAX_BIN_SEARCH_DEPTH; i++) {
        if (left > right) {
            break;
        }
        mid = left + (right - left) / 2;
        if (rows[mid].loc <= pc) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}

SEC("perf_event")
__s32 xm_do_perf_event(struct bpf_perf_event_data *ctx) {
    pid_t tid, pid;

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

    // 得到线程id
    tid = __xm_get_tid();
    // 得到进程id
    pid = __xm_get_pid();

    // perf的快照没有catch到执行线程
    if (tid == 0) {
        return 0;
    }
    // 只关注用户进程
    if (is_kthread()) {
        return 0;
    }

    // if (pid == 0) {
    //     bpf_printk("xm_do_perf_event this kernel thread:%d", tid);
    // }

    // 判断过滤条件
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    if (!filter_ts(&xm_profile_arg_map, ts, pid)) {
        return 0;
    }
    // bpf_printk("+++>xm_do_perf_event enter<+++");

    struct xm_task_userspace_regs tu_regs;
    bpf_user_pt_regs_t *user_regs = &ctx->regs;
    __xm_get_task_userspace_regs(ts, user_regs, &tu_regs);

    bpf_printk("---> xm_do_perf_event pid:%d", pid);
    bpf_printk("xm_do_perf_event rip:0x%lx, rsp:0x%lx, rbp:0x%x <---",
               tu_regs.rip, tu_regs.rsp, tu_regs.rbp);

    ps.pid = pid;
    BPF_CORE_READ_STR_INTO(&ps.comm, ts, group_leader, comm);

    // **如果执行程序支持frame point才能用这种方式，否则要用eh_frame +
    // **ip、bp、sp来做stack unwind
    int stack_id =
        bpf_get_stackid(ctx, &xm_profile_stack_map, BPF_F_USER_STACK);
    if (stack_id >= 0) {
        ps.user_stack_id = stack_id;
    }

    //  获取内核堆栈
    stack_id = bpf_get_stackid(ctx, &xm_profile_stack_map, KERN_STACKID_FLAGS);
    if (stack_id >= 0) {
        ps.kernel_stack_id = stack_id;
    }

    val = __xm_bpf_map_lookup_or_try_init((void *)&xm_profile_sample_count_map,
                                          &ps, &init_psd);
    if (val) {
        // 进程堆栈计数递增
        __sync_fetch_and_add(&(val->count), 1);
        val->pid_ns = __xm_get_task_pidns(ts);
        val->pgid = __xm_get_task_pgid(ts);
    }

    // bpf_printk("xm_do_perf_event pid:%d, kernel_stack_id:%d, "
    //            "user_stack_id:%d",
    //            ps.pid, ps.kernel_stack_id, ps.user_stack_id);

    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";