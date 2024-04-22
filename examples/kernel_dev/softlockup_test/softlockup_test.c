/*
 * @Author: CALM.WU
 * @Date: 2024-04-22 11:30:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-22 18:27:08
 */

#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h> // for kernel thread
#include <linux/err.h>
#include <linux/delay.h>

static struct task_struct *__cw_lock_holder_thread;

DEFINE_SPINLOCK(__cw_spinlock);
static uint8_t __cw_sleep_count = 60;
static uint8_t __cw_thread_exit_flag;

// soft lockup thread
static int32_t __cw_softlockup_lock_holder_thread_func(void *unused)
{
	pr_info("holder_thread:%d, Start\n", current->pid);

	spin_lock(&__cw_spinlock);
	// 测试是否持有 lock
	if (spin_is_locked(&__cw_spinlock)) {
		pr_info("holder_thread:%d, locked\n", current->pid);
	}

	do {
		msleep(1000);
		pr_info("holder_thread:%d, sleep %d\n", current->pid,
			__cw_sleep_count);
	} while (--__cw_sleep_count > 0 && !kthread_should_stop());

	spin_unlock(&__cw_spinlock);

	__cw_thread_exit_flag = 1;
	pr_info("holder_thread:%d, Exit\n", current->pid);
	return 0;
}

static int32_t __cw_softlockup_lock_acquirer_thread_func(void *unused)
{
	pr_info("acquirer_thread:%d, Start\n", current->pid);

	if (!spin_is_locked(&__cw_spinlock)) {
		pr_info("acquirer_thread:%d, not locked\n", current->pid);
	}

	spin_lock(&__cw_spinlock);
	if (spin_is_locked(&__cw_spinlock)) {
		pr_info("acquirer_thread:%d, locked\n", current->pid);
	}

	msleep(1000);

	spin_unlock(&__cw_spinlock);

	pr_info("acquirer_thread:%d, Exit\n", current->pid);
	return 0;
}

static int32_t __init __cw_softlockup_test_init(void)
{
	int32_t ret = 0;

	__cw_lock_holder_thread =
		kthread_run(__cw_softlockup_lock_holder_thread_func, NULL,
			    "cw_softlockup_holder_thread");
	if (IS_ERR(__cw_lock_holder_thread)) {
		ret = PTR_ERR(__cw_lock_holder_thread);
		pr_err("Failed to create 'cw_lock_holder_thread' thread (%d)\n",
		       ret);
		return ret;
	}

	msleep(500);

	kthread_run(__cw_softlockup_lock_acquirer_thread_func, NULL,
		    "cw_softlockup_acquirer_thread");

	pr_info("init successfully\n");
	return ret;
}

static void __exit __cw_softlockup_test_exit(void)
{
	// 如果线程主动退出，调用 kthread_stop 会 crash
	if (!__cw_thread_exit_flag && !IS_ERR(__cw_lock_holder_thread)) {
		kthread_stop(__cw_lock_holder_thread);
	}
	pr_info("bye\n");
}

module_init(__cw_softlockup_test_init);
module_exit(__cw_softlockup_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("soft lockup Example");
MODULE_VERSION("0.1");