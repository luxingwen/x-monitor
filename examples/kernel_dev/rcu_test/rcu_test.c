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

struct foo {
    s32             a;
    struct rcu_head rcu;
};

static struct foo *g_ptr = NULL;

// read thread
static void __myrcu_header_thread(void *data) {
    struct foo *p = NULL;

    while (1) {
        msleep(200);
        rcu_read_lock();
        p = rec_dereference(g_ptr);
        if (p) {
            printk("%s: read a = %d\n", __func__, p->a);
        }
        rcu_read_unlock();
    }
}

static void __myrcu_del(struct rcu_head *rh) {
    struct foo *p = container_of(rh, struct foo, rcu);
}
