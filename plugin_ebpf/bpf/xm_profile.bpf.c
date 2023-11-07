/*
 * @Author: CALM.WU
 * @Date: 2023-08-16 15:15:40
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-11-03 17:30:12
 */

#include <vmlinux.h>
#include "../bpf_and_user.h"
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_math.h"
#include "xm_bpf_helpers_filter.h"

// 在 FDETable 中二分查找对应 row 的最大深度，那么一个 FDETable 最多支持 2^8=256
// 有 row
#define MAX_FDE_TABLE_ROW_BIN_SEARCH_DEPTH 8
// 在 Module 中查找 FDEInfo
// 的最大深度，XM_PER_MODULE_FDE_TABLE_COUNT=8192，最大深度是 13
#define MAX_MODULE_FDE_TABLE_BIN_SEARCH_DEPTH 13

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

// prog 参数，过滤条件
BPF_ARRAY(xm_profile_arg_map, struct xm_prog_filter_args, 1);
// 堆栈计数
BPF_HASH(xm_profile_sample_count_map, struct xm_profile_sample,
         struct xm_profile_sample_data, MAX_THREAD_COUNT);
// 堆栈
BPF_STACK_TRACE(xm_profile_stack_map, 1024);
// 保存 pid 的内存映射信息，如果 pid 存在，说明需要执行来获取用户态堆栈
BPF_HASH(xm_profile_pid_modules_map, __u32, struct xm_profile_pid_maps, 256);
// 解决：error: Looks like the BPF stack limit of 512 bytes is
// exceeded，因为结构太大，不能作为局部变量
BPF_PERCPU_ARRAY(xm_profile_stack_trace_heap, struct xm_dwarf_stack_trace, 1);

// 保存每个 module 的 fde table 信息，内核对 hash_map 的 value 有大小限制
// if ((u64)attr->key_size + attr->value_size >= KMALLOC_MAX_SIZE -
//    sizeof(struct htab_elem))
// 	 if key_size + value_size is bigger, the user space won't be
// 	 * able to access the elements via bpf syscall. This check
// 	 * also makes sure that the elem_size doesn't overflow and it's
// 	 * kmalloc-able later in htab_map_update_elem()
//
// 	return -E2BIG;
// key is hash(buildid)
BPF_HASH(xm_profile_module_fdetables_map, __u64,
         struct xm_profile_module_fde_tables, 256);

// in compile print struct sizeof
char (*__kaboom)[sizeof(struct xm_profile_module_fde_tables)] = 1;

const enum xm_prog_filter_target_scope_type __unused_filter_scope_type
    __attribute__((unused)) = XM_PROG_FILTER_TARGET_SCOPE_TYPE_NONE;

const struct xm_profile_sample *__unused_ps __attribute__((unused));
const struct xm_profile_sample_data *__unused_psd __attribute__((unused));
const struct xm_profile_fde_table_row *__unused_pfr __attribute__((unused));
const struct xm_profile_fde_table_info *__unused_pft __attribute__((unused));
const struct xm_profile_proc_maps_module *__unused_pmm __attribute__((unused));
const struct xm_profile_pid_maps *__unused_pm __attribute__((unused));
const struct xm_profile_module_fde_tables *__unused_pmft
    __attribute__((unused));

static const __u64 __zero = 0;

// 获取用户 task_struct 用户空间的寄存器，为了使用 eh_frame 的信息做 stack
// unwind
static __always_inline void
__xm_get_task_userspace_regs(struct task_struct *task,
                             struct pt_regs *user_regs,
                             struct xm_task_userspace_regs *tu_regs) {
    // 获取指令寄存器的值，判断了是否是内核地址空间
    if (in_kernel_space(PT_REGS_IP(user_regs))) {
        // !! 由于系统调用在进程切换到内核地址空间，需要按 ptrace
        // 的方式，来获取保存在 stack 中的用户态寄存器
        __u64 __ptr;
        __ptr = (__u64)READ_KERN(task->stack);
        __ptr += THREAD_SIZE - TOP_OF_KERNEL_STACK_PADDING;
        struct pt_regs *regs = ((struct pt_regs *)__ptr) - 1;
        tu_regs->rip = PT_REGS_IP_CORE(regs);
        tu_regs->rsp = PT_REGS_SP_CORE(regs);
        tu_regs->rbp = PT_REGS_FP_CORE(regs);
    } else {
        // perf event
        // snapshot 当前 task_struct 在用户地址空间，直接从 cur_regs 中获取
        tu_regs->rip = PT_REGS_IP(user_regs);
        tu_regs->rsp = PT_REGS_SP(user_regs);
        tu_regs->rbp = PT_REGS_FP(user_regs);
    }
}

// int element = sortedArray[index];
// !!R8 unbounded memory access, make sure to bounds check any such access
static __always_inline __s32
__bsearch_fde_table_row(const struct xm_profile_fde_table_row *rows, __u32 left,
                        __u32 right, __u64 address) {
    __s32 result = -1;
    __s32 mid = 0;
    // 最大深度限制查找 rows 的范围
    for (__u32 i = 0; i < MAX_FDE_TABLE_ROW_BIN_SEARCH_DEPTH; i++) {
        if (left > right) {
            break;
        }
        mid = left + (right - left) / 2;

        if (mid < 0 || mid >= XM_PER_MODULE_FDE_ROWS_COUNT) {
            return -1;
        }

        if (rows[mid].loc <= address) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}

static __always_inline const struct xm_profile_fde_table_info *
__find_fde_table_info_in_module(
    const struct xm_profile_module_fde_tables *module_tables, __u64 address) {
    if (module_tables->fde_table_count < XM_PER_MODULE_FDE_TABLE_COUNT) {
        __s32 left = 0;
        __s32 right = module_tables->fde_table_count - 1;
        __s32 result = -1;
        __s32 mid = 0;
        const struct xm_profile_fde_table_info *fde_table_infos =
            module_tables->fde_infos;

        for (__s32 i = 0; i < MAX_MODULE_FDE_TABLE_BIN_SEARCH_DEPTH; i++) {
            if (left > right) {
                break;
            }
            mid = left + (right - left) / 2;

            // **fix:math between map_value pointer and register with unbounded
            // **min value is not allowed
            if (mid < 0 || mid >= XM_PER_MODULE_FDE_TABLE_COUNT) {
                return 0;
            }
            if (fde_table_infos[mid].start <= address) {
                result = mid;
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }

        if (result >= 0) {
            return &module_tables->fde_infos[result];
        }
    }
    return 0;
}

static __always_inline const struct xm_profile_proc_maps_module *
__find_module_in_maps(const struct xm_profile_pid_maps *maps, __u64 pc) {
    for (__u32 i = 0; i < XM_PER_PROCESS_ASSOC_MODULE_COUNT; i++) {
        const struct xm_profile_proc_maps_module *module = &maps->modules[i];
        if (module->start_addr <= pc && pc <= module->end_addr) {
            // 判断 pc 在/proc/pid/maps 对应的 module 地址范围内
            return module;
        }
    }

    return 0;
}

static __always_inline const struct xm_profile_fde_table_row *
__find_fde_table_row_by_pc(const struct xm_profile_pid_maps *pid_maps,
                           __u64 pc) {
    const struct xm_profile_proc_maps_module *module = 0;
    const struct xm_profile_module_fde_tables *module_fde_tables = 0;
    const struct xm_profile_fde_table_info *module_fde_table_info = 0;
    __u64 addr = pc;
    __s32 find_row_pos = 0;

    // 根据 rip 查找对应的 module
    module = __find_module_in_maps(pid_maps, pc);
    if (module) {
        bpf_printk("find module-{start:0x%lx...end:0x%lx}, type:%d",
                   module->start_addr, module->end_addr, module->type);

        if (module->type == so) {
            addr -= module->start_addr;
            bpf_printk("module type is 'so', adjust (addr-=module->start_addr) "
                       "==> 0x%lx",
                       addr);
        }
        // 根据 module 的 build_id_hash 查找对应的 module
        module_fde_tables = bpf_map_lookup_elem(
            &xm_profile_module_fdetables_map, &(module->build_id_hash));

        if (module_fde_tables) {
            bpf_printk("module build_id_hash:'%llu' in "
                       "xm_profile_module_fde_tables",
                       module->build_id_hash);

            // 根据 addr 查找对应的 fde table
            module_fde_table_info =
                __find_fde_table_info_in_module(module_fde_tables, addr);
            if (module_fde_table_info) {
                bpf_printk("addr:0x%lx in "
                           "fde_table-{start:0x%lx...end:0x%lx}",
                           addr, module_fde_table_info->start,
                           module_fde_table_info->end);

                // 根据 addr 查找对应的 fde table row
                if ((module_fde_table_info->row_pos
                     + module_fde_table_info->row_count)
                    <= XM_PER_MODULE_FDE_ROWS_COUNT) {
                    find_row_pos = __bsearch_fde_table_row(
                        module_fde_tables->fde_rows,
                        module_fde_table_info->row_pos,
                        module_fde_table_info->row_pos
                            + module_fde_table_info->row_count - 1,
                        addr);
                    if (find_row_pos >= 0
                        && find_row_pos < XM_PER_MODULE_FDE_ROWS_COUNT) {
                        bpf_printk("addr:0x%lx at fde_table %d row.", addr,
                                   find_row_pos
                                       - module_fde_table_info->row_pos);
                        return &module_fde_tables->fde_rows[find_row_pos];
                    } else {
                        bpf_printk("addr:0x%lx does not have a "
                                   "corresponding row in the fde_table.",
                                   addr);
                    }
                }
            }
        } else {
            bpf_printk("module-{start:0x%lx...end:0x%lx}, build_id_hash:%d "
                       "not in xm_profile_module_fde_tables",
                       module->start_addr, module->end_addr,
                       module->build_id_hash);
        }
    }
    return 0;
}

static __always_inline __s32 __get_user_stack(
    const struct xm_profile_pid_maps *pid_maps, struct task_struct *task,
    struct pt_regs *user_regs, struct xm_dwarf_stack_trace *st) {
    struct xm_task_userspace_regs tu_regs;
    const struct xm_profile_fde_table_row *row = 0;
    __u64 rip = 0, prev_rip_addr = 0, rsp = 0, rbp = 0, prev_rbp_addr = 0,
          cfa = 0;
    __s32 ret = 0;
    bool reached_bottom = false;

    // 获取当前的寄存器
    __xm_get_task_userspace_regs(task, user_regs, &tu_regs);
    bpf_printk("rip:0x%lx, rsp:0x%lx, rbp:0x%x", tu_regs.rip, tu_regs.rsp,
               tu_regs.rbp);
    // 这是 top frame 当前执行的指令地址
    rip = tu_regs.rip;
    rsp = tu_regs.rsp;
    rbp = tu_regs.rbp;
    st->len = 0;
    st->pc[0] = rip;

    for (__s32 i = 1; i < XM_MAX_STACK_DEPTH_PER_PROGRAM; i++) {
        bpf_printk("\t*** find rip:0x%lx ***", rip);
        row = __find_fde_table_row_by_pc(pid_maps, rip);
        if (row) {
            // 找到 pc 在 FDETables 中对应的 row
            bpf_printk("\t***rip correspond with row loc:0x%lx cfa{reg_num:%d, "
                       "offset:%d}",
                       row->loc, row->cfa.reg, row->cfa.offset);
            bpf_printk("\t   rbp_cfa_offset:%d, ra_cfa_offset:%d***",
                       row->rbp_cfa_offset, row->ra_cfa_offset);

            // 判断是否到了 top frame

            // 开始计算 cfa，也是上一帧的 rsp
            // AMD64_Rbp     = 6
            // AMD64_Rsp     = 7
            if (row->cfa.reg == 7) {
                cfa = rsp + row->cfa.offset;
            } else if (row->cfa.reg == 6) {
                cfa = rbp + row->cfa.offset;
            } else {
                ret = -1;
                bpf_printk("   loc:0x%lx, cfa reg:%d not support.", row->loc,
                           row->cfa.reg);
                break;
            }
            bpf_printk("\t   cfa:0x%lx", cfa);

            if (row->rbp_cfa_offset != INT_MAX) {
                // 通过 cfa + rbp_cfa_offset 来计算 上一帧 rbp
                // 在栈空间的地址
                prev_rbp_addr = cfa + row->rbp_cfa_offset;
                bpf_printk("\t   prev_rbp_addr:0x%lx", prev_rbp_addr);
                // 读取上一帧 rbp 的值
                ret = bpf_probe_read_user(&rbp, 8, (void *)prev_rbp_addr);
                if (ret < 0) {
                    bpf_printk("\t   [error] read previous rbp at "
                               "prev_rbp_addr:0x%lx failed. err:%d",
                               prev_rbp_addr, ret);
                    break;
                } else {
                    bpf_printk("\t   previous rbp:0x%lx, prev_rbp_addr:0x%lx",
                               rbp, prev_rbp_addr);
                }
            } else {
                reached_bottom = true;
            }

            if (row->ra_cfa_offset != INT_MAX) {
                // 通过 cfa + ra_cfa_offset 来计算上一帧 rip
                prev_rip_addr = cfa + row->ra_cfa_offset;
                bpf_printk("\t   prev_rip_addr:0x%lx", prev_rip_addr);
                // 读取上一帧 rip 的值
                ret = bpf_probe_read_user(&rip, 8, (void *)prev_rip_addr);
                if (ret < 0) {
                    bpf_printk("\t   [error] read previous rip at "
                               "prev_rip_addr:0x%lx failed. err:%d",
                               prev_rip_addr, ret);
                    break;
                } else {
                    bpf_printk("\t   previous rip:0x%lx, prev_rip_addr:0x%lx",
                               rip, prev_rip_addr);
                }
            } else {
                reached_bottom = true;
            }

            st->pc[i] = rip;
            st->len += 1;

            if (!reached_bottom) {
                rsp = cfa;
            } else {
                // 到了最低的 call frame
                bpf_printk("\t   reached bottom, stack depth:%d", st->len);
                break;
            }
        } else {
            break;
        }
    }

    return ret;
}

SEC("perf_event") __s32 xm_do_perf_event(struct bpf_perf_event_data *ctx) {
    pid_t tid, pid;
    struct xm_profile_pid_maps *pid_maps = 0;

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

    // 得到线程 id
    tid = __xm_get_tid();
    // 得到进程 id
    pid = __xm_get_pid();

    // perf 的快照没有 catch 到执行线程
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
    // struct xm_task_userspace_regs tu_regs;
    // bpf_user_pt_regs_t *user_regs = &ctx->regs;
    //__xm_get_task_userspace_regs(ts, user_regs, &tu_regs);
    // bpf_printk("---> xm_do_perf_event pid:%d", pid);
    // bpf_printk("xm_do_perf_event rip:0x%lx, rsp:0x%lx, rbp:0x%x <---",
    //            tu_regs.rip, tu_regs.rsp, tu_regs.rbp);

    ps.pid = pid;
    BPF_CORE_READ_STR_INTO(&ps.comm, ts, group_leader, comm);

    // **如果执行程序支持 frame point 才能用这种方式，否则要用 eh_frame +
    // **ip、bp、sp 来做 stack unwind
    // 判断是否需要通过解析 eh_frame 的方式来做 unwind stack
    pid_maps = bpf_map_lookup_elem(&xm_profile_pid_modules_map, &pid);
    struct xm_dwarf_stack_trace *st =
        bpf_map_lookup_elem(&xm_profile_stack_trace_heap, &__zero);
    if (pid_maps && st) {
        bpf_printk("---> __get_user_stack pid:%d", pid);
        __get_user_stack(pid_maps, ts, &ctx->regs, st);
        bpf_printk("<---\n");
    }

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

    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";

// !! processed 57970 insns (limit 1000000) max_states_per_insn 4 total_states
// !! 1110 peak_states 1109 mark_read 36

// !! The sequence of 8193 jumps is too complex.

// !! processed 1000001 insns (limit 1000000) max_states_per_insn 19
// !! total_states
// !! 20748 peak_states 1257 mark_read 33
