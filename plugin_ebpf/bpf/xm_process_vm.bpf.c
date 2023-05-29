/*
 * @Author: CALM.WU
 * @Date: 2023-04-26 14:51:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-27 15:07:26
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"

#include "../bpf_and_user.h"

#define MAP_SHARED 0x01 /* Share changes */
#define MAP_PRIVATE 0x02 /* Changes are private */
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20 /* don't use a file */

#define PROT_READ 0x1 /* page can be read */
#define PROT_WRITE 0x2 /* page can be written */
#define PROT_EXEC 0x4 /* page can be executed */
#define PROT_SEM 0x8 /* page may be used for atomic ops */

// **全局变量：过滤条件
// 过滤范围，1:os，2：namespace，3：CGroup，4：PID，5：PGID，暂时不支持cg
const volatile __s32 __filter_scope_type = 1;
// 范围具体值，例如pidnsID, pid, pgid，如果scope_type为1，表示范围为整个os
const volatile __s64 __filter_scope_value = 0;

// !! Force emitting struct event into the ELF. 切记不能使用static做修饰符
const struct xm_processvm_evt_data *__unused_vm_evt __attribute__((unused));

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24); // 16M
} xm_processvm_event_ringbuf_map SEC(".maps");

//**如果type是__u32，整个结构会有空洞，如果不memset，加载器会报错，导致加载失败
struct vm_op_info {
    enum xm_processvm_evt_type type;
    __u64 addr;
    __u64 len;
    __u64 flags;
};

BPF_HASH(vm_op_map, __u32, struct vm_op_info, 4096);

static __s32 __filter_check(struct task_struct *ts) {
    pid_t tgid = READ_KERN(ts->tgid);

    if (__filter_scope_type == 2) {
        __u32 pidns_id = __xm_get_task_pidns(ts);
        if (pidns_id != (pid_t)__filter_scope_value) {
            return -1;
        }
    } else if (__filter_scope_type == 4) {
        if (tgid != (pid_t)__filter_scope_value) {
            return -1;
        }
    } else if (__filter_scope_type == 5) {
        pid_t pgid = __xm_get_task_pgid(ts);
        if (pgid != (pid_t)__filter_scope_value) {
            return -1;
        }
    }
    return 0;
}

static void __insert_processvm_event(struct vm_op_info *voi) {
    struct xm_processvm_evt_data *evt =
        bpf_ringbuf_reserve(&xm_processvm_event_ringbuf_map,
                            sizeof(struct xm_processvm_evt_data), 0);
    if (evt) {
        struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
        evt->tid = __xm_get_tid();
        evt->pid = __xm_get_pid();
        evt->addr = voi->addr;
        evt->len = voi->len;
        evt->evt_type = voi->type;
        bpf_core_read_str(evt->comm, sizeof(evt->comm), &ts->comm);
        // !!如果evt放在submit后面调用，就会被检查出内存无效
        // bpf_printk("xm_ebpf_exporter vma alloc event type:%d tgid:%d
        // len:%lu\n",
        //            evt_type, evt->tgid, evt->len);
        bpf_ringbuf_submit(evt, 0);
    }
    return;
}
#if 0
SEC("kprobe/__vma_adjust")
__s32 xm_process___vma_adjust(struct pt_regs *ctx) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        bpf_printk("xm_ebpf_exporter __vma_adjust pid:%d, op:'%d', "
                   "len:%lu",
                   pid, voi->type, voi->len);
    }
    return 0;
}

SEC("kprobe/vm_area_alloc")
__s32 xm_process_vm_area_alloc(struct pt_regs *ctx) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        bpf_printk("xm_ebpf_exporter vm_area_alloc pid:%d, op:'%d', "
                   "len:%lu bytes",
                   pid, voi->type, voi->len);
    }
    return 0;
}

SEC("kretprobe/vma_merge")
__s32 BPF_KRETPROBE(kretprobe__xm_vma_merge, struct vm_area_struct *vma) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        // 如果返回值vma不为空，表示合并成功，vma进行了扩展
        if (vma) {
            __u64 vma_start = 0, vma_end = 0, vma_size = 0;
            BPF_CORE_READ_INTO(&vma_start, vma, vm_start);
            BPF_CORE_READ_INTO(&vma_end, vma, vm_end);
            vma_size = vma_end - vma_start;
            bpf_printk("kretprobe__xm_vma_merge pid:%d, op:'%d'", pid,
                       voi->type);
            bpf_printk("len:%lu bytes, vma start:%lu, size:%lu, merge "
                       "successes.\n",
                       voi->len, vma_start, vma_size);
            __insert_processvm_event(voi->type, vma_size);
        } else {
            bpf_printk("kretprobe__xm_vma_merge pid:%d, op:'%d', "
                       "len:%lu bytes, return NULL\n",
                       pid, voi->type, voi->len);
        }
        bpf_map_delete_elem(&vm_op_map, &tid);
    }
    return 0;
}

SEC("kprobe/vma_merge")
__s32 kprobe__xm_vma_merge(struct pt_regs *ctx) {
    __u32 tid = __xm_get_tid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        __u64 addr = (__u64)PT_REGS_PARM3(ctx);
        __u64 end = (__u64)PT_REGS_PARM4(ctx);
        bpf_printk("kprobe__xm_vma_merge pid:%d, addr:0x%lx, end:0x%lx", pid,
                   addr, end);
    }
    return 0;
}
#endif

SEC("kprobe/mmap_region")
__s32 kprobe__xm_mmap_region(struct pt_regs *ctx) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        voi->addr = (__u64)PT_REGS_PARM2(ctx);
        voi->len = (__u64)PT_REGS_PARM3(ctx);
        bpf_printk("kprobe__xm_mmap_region pid:%d, addr:0x%lx, len:%lu", pid,
                   voi->addr, voi->len);
    }
    return 0;
}

SEC("kprobe/do_mmap")
__s32 kprobe__xm_do_mmap(struct pt_regs *ctx) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();

    if (0 == __filter_check(ts)) {
        struct file *pf = (struct file *)PT_REGS_PARM1_CORE(ctx);
        __u64 len = (__u64)PT_REGS_PARM3_CORE(ctx);
        // 表示映射区域的保护权限，可以是PROT_NONE、PROT_READ、PROT_WRITE、PROT_EXEC，或它们的组合
        __u64 prot = (__u64)PT_REGS_PARM4_CORE(ctx);
        // flags：表示映射区域的标志，可以是
        // MAP_SHARED、MAP_PRIVATE、MAP_FIXED、MAP_ANONYMOUS、MAP_LOCKED 等。
        __u64 flags = (__u64)PT_REGS_PARM5_CORE(ctx);
        struct vm_op_info voi;
        __builtin_memset((void *)&voi, 0, sizeof(voi));

        char comm[TASK_COMM_LEN];
        bpf_get_current_comm(&comm, sizeof(comm));

        if (!pf && (prot & PROT_WRITE)
            && (((flags & MAP_FIXED) == 0)
                && ((flags & (MAP_PRIVATE | MAP_ANONYMOUS)) != 0))) {
            // !! 奇怪如果有MAP_FIXED，那么kretprobe就不会捕捉
            // 使用mmap分配的私有匿名虚拟地址空间
            voi.len = len;
            voi.type = XM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV;
            bpf_map_update_elem(&vm_op_map, &tid, &voi, BPF_ANY);
            bpf_printk("kprobe__xm_do_mmap MMAP_ANON_PRIV comm:'%s', "
                       "pid:%d, len:%lu",
                       comm, pid, len);
        } else if (pf && (prot & PROT_WRITE) && (flags & MAP_SHARED)) {
            // 使用mmap做文件映射
            voi.len = len;
            voi.type = XM_PROCESSVM_EVT_TYPE_MMAP_SHARED;
            bpf_map_update_elem(&vm_op_map, &tid, &voi, BPF_ANY);
            bpf_printk("kprobe__xm_do_mmap MMAP_SHARED comm:'%s', "
                       "pid:%d, len:%lu",
                       comm, pid, len);
        }
    }
    return 0;
}

SEC("kretprobe/do_mmap")
__s32 BPF_KRETPROBE(kretprobe__xm_do_mmap, unsigned long ret) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();

    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        if (!IS_ERR_VALUE(ret)) {
            // do_mmap成功
            // bpf_printk("kretprobe__xm_do_mmap pid:%d, addr:0x%lx, ret:0x%lx",
            //            pid, voi->addr, ret);
            __insert_processvm_event(voi);
        }
        bpf_map_delete_elem(&vm_op_map, &tid);
    }
    return 0;
}

#if 0
SEC("kprobe/do_brk_flags")
__s32 xm_process_do_brk_flags(struct pt_regs *ctx) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    // pid_t tgid = __xm_get_pid();
    // request：表示要增加或减少的内存大小。如果该值为
    // 0，则表示不需要增加或减少内存大小，只需要查看当前数据段的结束地址。

    if (0 == __filter_check(ts)) {
        __u64 addr = (__u64)PT_REGS_PARM1_CORE(ctx);
        __u64 len = (__u64)PT_REGS_PARM2_CORE(ctx);

        char comm[TASK_COMM_LEN];
        bpf_get_current_comm(&comm, sizeof(comm));

        struct vm_op_info voi;
        __builtin_memset((void *)&voi, 0, sizeof(voi));
        voi.addr = addr;
        voi.len = len;
        voi.type = XM_PROCESSVM_EVT_TYPE_BRK;
        bpf_map_update_elem(&vm_op_map, &tid, &voi, BPF_ANY);
        bpf_printk("xm_ebpf_exporter do_brk_flags comm:'%s', pid:%d", comm,
                   pid);
        bpf_printk("addr:%lx, len:%lu", addr, len);
    }

    return 0;
}

SEC("kretprobe/do_brk_flags")
__s32 BPF_KRETPROBE(xm_process_do_brk_flags_ret, __s32 ret) {
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);

    if (voi) {
        // 返回成功，触发事件
        if (ret > 0) {
            __insert_processvm_event(voi);
            bpf_printk("xm_ebpf_exporter do_brk_flags_ret pid:%d ret:%d\n", pid,
                       ret);
        }
        bpf_map_delete_elem(&vm_op_map, &tid);
    }
    return 0;
}

// !!不知道为何kprobe无法获取系统调用的参数，只有换为tracepoint
// SEC("kprobe/" SYSCALL(sys_brk))
// __s32 BPF_KPROBE(xm_process_sys_brk, __u64 brk) {
SEC("tracepoint/syscalls/sys_enter_brk")
__s32 xm_process_sys_enter_brk(struct trace_event_raw_sys_enter *ctx) {
    __u64 orig_brk = 0;
    //__u64 start_brk = 0;
    __u32 tid = __xm_get_tid();

    // 获取要设置堆顶地址
    __u64 new_brk = (__u64)ctx->args[0];
    if (new_brk == 0) {
        return 0;
    }

    // 获取task
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    if (0 == __filter_check(ts)) {
        // 读取task的堆顶地址
        // start_brk = (__u64)BPF_CORE_READ(ts, mm, start_brk);
        orig_brk = (__u64)BPF_CORE_READ(ts, mm, brk);

        // 判断进程堆顶地址
        if (new_brk <= orig_brk) {
            // bpf_printk("xm_ebpf_exporter xm_process_sys_brk tid:%d, "
            //            "new_brk:%lu <= orig_brk:%lu",
            //            tid, new_brk, orig_brk);
            // kernel会调用__do_munmap，用线程id作为标识
            // !!map的数据类型必须完全对应，否则报错
            struct vm_op_info voi;
            __builtin_memset((void *)&voi, 0, sizeof(voi));
            voi.len = 0;
            voi.type = XM_PROCESSVM_EVT_TYPE_BRK_SHRINK;
            bpf_map_update_elem(&vm_op_map, &tid, &voi, BPF_ANY);
        }
    }
    return 0;
}
#endif

SEC("kprobe/__do_munmap")
__s32 kprobe__xm___do_munmap(struct pt_regs *ctx) {
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();

    if (0 == __filter_check(ts)) {
        // 第三个参数堆释放的长度
        __u32 tid = __xm_get_tid();
        unsigned long addr = (unsigned long)PT_REGS_PARM2_CORE(ctx);
        __u64 len = (__u64)PT_REGS_PARM3_CORE(ctx);
        struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
        if (voi) {
            // bpf_printk("xm_ebpf_exporter xm_process_do_munmap tid:%d, "
            //            "new_brk:0x%p, len:%lu",
            //            tid, *res, len);
            // bpf_map_update_elem(&vm_op_map, &tid, &len, BPF_EXIST);
            __sync_fetch_and_add(&(voi->len), len);
        } else {
            struct vm_op_info temp;
            __builtin_memset((void *)&temp, 0, sizeof(voi));
            temp.addr = addr;
            temp.len = len;
            temp.type = XM_PROCESSVM_EVT_TYPE_MUNMAP;
            temp.flags = 0;
            bpf_map_update_elem(&vm_op_map, &tid, &temp, BPF_NOEXIST);
        }
    }

    return 0;
}

SEC("kretprobe/__do_munmap")
__s32 BPF_KRETPROBE(kretprobe__xm___do_munmap, __s32 ret) {
    // sys_x86_64.c __do_munmap return downgrade ? 1 : 0;
    __u32 tid = __xm_get_tid();
    __u32 pid = __xm_get_pid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        __u64 flags = voi->flags;
        if (ret == 1) {
            bpf_printk("kretprobe__xm___do_munmap pid:%d, flags:%lx", pid,
                       flags);
            bpf_printk("kretprobe__xm___do_munmap op:'%d', addr:0x%lx, "
                       "len:%lu",
                       voi->type, voi->addr, voi->len);
            __insert_processvm_event(voi);
        }
        bpf_map_delete_elem(&vm_op_map, &tid);
    }
    return 0;
}

SEC("kprobe/unmap_region")
__s32 kprobe__xm_unmap_region(struct pt_regs *ctx) {
    __u32 tid = __xm_get_tid();
    struct vm_op_info *voi = bpf_map_lookup_elem(&vm_op_map, &tid);
    if (voi) {
        // 读取vma flags
        struct vm_area_struct *vma =
            (struct vm_area_struct *)PT_REGS_PARM2_CORE(ctx);
        voi->flags = (__u64)BPF_CORE_READ(vma, vm_flags);
    }

    return 0;
}

// ** GPL
char LICENSE[] SEC("license") = "Dual BSD/GPL";