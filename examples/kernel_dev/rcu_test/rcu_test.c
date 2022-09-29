/*
 * @Author: CALM.WU
 * @Date: 2022-09-25 11:43:24
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-25 12:24:31
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define AUTHOR "calm.wu <wubo0067@hotmail.com>"
#define DESC "RCU test"

struct foo {
    s32             a;
    struct rcu_head rcu;
};

static struct foo         *g_ptr = NULL;
static struct task_struct *read_thread = NULL;
static struct task_struct *write_thread[2] = { NULL, NULL };

DEFINE_SPINLOCK(foo_mutex);

// read thread
static s32 __myrcu_header_thread(void *data) {
    struct foo *p = NULL;

    while (!kthread_should_stop()) {
        ssleep(1);
        rcu_read_lock();
        // 从全局变量中读取
        p = rcu_dereference(g_ptr);
        if (p) {
            printk("thread: %s:%d %s: read a = %d\n", current->comm, current->pid, __func__, p->a);
        }
        // 宽限期结束
        rcu_read_unlock();
    }
    return 0;
}

static void __myrcu_del(struct rcu_head *rh) {
    struct foo *p = container_of(rh, struct foo, rcu);
    printk("%s: a=%d\n", __func__, p->a);
    kfree(p);
}

static s32 __myrcu_writer_thread(void *p) {
    struct foo *old_fp;
    struct foo *new_fp;

    s32 value = *(s32 *)p;

    while (!kthread_should_stop()) {
        ssleep(3);
        new_fp = (struct foo *)kmalloc(sizeof(struct foo), GFP_KERNEL);
        spin_lock(&foo_mutex);

        printk("thread: %s:%d %s: write to new %d\n", current->comm, current->pid, __func__, value);
        // 保留old_fp，这个指针的释放需要由synchronize_rcu和call_rcu来决定
        old_fp = g_ptr;
        *new_fp = *old_fp;
        new_fp->a = value;

        // 全局数据更新，实际read获得已经是全局数据的过期数据了
        rcu_assign_pointer(g_ptr, new_fp);

        spin_unlock(&foo_mutex);

        if (old_fp) {
            // 保证读的数据在读完之前不能被释放
            call_rcu(&old_fp->rcu, __myrcu_del);
        }
        value++;
    }
    return 0;
}

static s32 __init myrcu_test_init(void) {

    s32 value_1 = 700;
    s32 value_2 = 90000;

    printk("calm-go: myrcu_test module init\n");
    g_ptr = kzalloc(sizeof(struct foo), GFP_KERNEL);

    // 启动内核线程
    read_thread = kthread_run(__myrcu_header_thread, NULL, "myrcu_rd_thd");
    write_thread[0] = kthread_run(__myrcu_writer_thread, (void *)&value_1, "myrcu_wr_thd_1");
    write_thread[1] = kthread_run(__myrcu_writer_thread, (void *)&value_2, "myrcu_wr_thd_2");

    return 0;
}

static void __exit myrcu_test_exit(void) {
    printk("calm-go: myrcu_test module exit\n");
    kthread_stop(read_thread);
    kthread_stop(write_thread[0]);
    kthread_stop(write_thread[1]);

    if (g_ptr) {
        kfree(g_ptr);
    }
}

module_init(myrcu_test_init);
module_exit(myrcu_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
