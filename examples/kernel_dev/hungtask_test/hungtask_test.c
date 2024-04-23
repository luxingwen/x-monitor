/*
 * @Author: CALM.WU
 * @Date: 2024-04-23 14:45:01
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-23 15:56:29
 */

#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/sched/task.h> // {get,put}_task_struct()

static struct task_struct *__cw_hung_task_thread;
static int64_t __cw_sleep_secs = 200;

static int32_t __cw_hung_task_thread_func(void *unused)
{
	pr_info("hung_task_thread:%d, Start\n", current->pid);
	// 设置为不可中断
	set_current_state(TASK_UNINTERRUPTIBLE);
	// 睡眠 150s，触发 watchdog
	schedule_timeout(__cw_sleep_secs * HZ);
	// 被唤醒或者超时后
	pr_info("hung_task_thread:%d, awake up\n", current->pid);
	// 模拟工作
	msleep(1000);
	pr_info("hung_task_thread:%d, Exit\n", current->pid);
	return 0;
}

static int32_t __init __cw_hung_task_test_init(void)
{
	__cw_hung_task_thread = kthread_run(__cw_hung_task_thread_func, NULL,
					    "cw_hung_task_thread");
	if (IS_ERR(__cw_hung_task_thread)) {
		pr_err("Failed to start 'cw_hung_task_thread' thread\n");
		return PTR_ERR(__cw_hung_task_thread);
	}
	pr_info("Hello, hung task test module\n");
	return 0;
}

static void __exit __cw_hung_task_test_exit(void)
{
	if (!IS_ERR(__cw_hung_task_thread) &&
	    // oracle linux 4.18 state 改为__state
	    (__cw_hung_task_thread->__state == TASK_RUNNING)) {
		// 主动终止线程
		pr_info("kthread_stop hung_task_thread\n");
		// 在调用 kthread_stop 之前，一定要检查需要被终止的线程是否还在运行。如果 kthread_stop 调用了一个未运行的线程，可能会导致程序崩溃
		kthread_stop(__cw_hung_task_thread);
	}

	pr_info("Goodbye, hung task test module\n");
}

module_init(__cw_hung_task_test_init);
module_exit(__cw_hung_task_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("hung task Example");
MODULE_VERSION("0.1");
