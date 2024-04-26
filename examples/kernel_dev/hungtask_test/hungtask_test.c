/*
 * @Author: CALM.WU
 * @Date: 2024-04-23 14:45:01
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-26 17:22:15
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
#include <linux/sched/signal.h>
#include "../misc.h"

#define SCHED_NORMAL 0
#define SCHED_TIMEOUT 1
#define SCHED_TYPE SCHED_NORMAL

static struct task_struct *__cw_hung_task_thread;
static int64_t __cw_sleep_secs = 200;

static int32_t __cw_hung_task_thread_func(void *unused)
{
	int64_t ret = 0;

	PRINT_CTX();
	if (!current->mm) {
		// 内核线程
		pr_info("hung_task_thread:%d start, we are kernel thread\n",
			current->pid);
	}

	// 默认情况下所有的信号都是屏蔽的对内核线程
	// 等唤醒后可用 signal_pending 函数来判断是否收到信号
	// !! 但信号是无法唤醒 task 的
	allow_signal(SIGQUIT);
	allow_signal(SIGINT);

	// 设置为不可中断
	set_current_state(TASK_UNINTERRUPTIBLE);
	// 睡眠 __cw_sleep_secs，会触发 watchdog
	pr_info("hung_task_thread:%d is going to sleep %lld seconds\n",
		current->pid, __cw_sleep_secs);

#if (SCHED_TYPE == SCHED_TIMEOUT)
	ret = schedule_timeout(__cw_sleep_secs * HZ);

	if (signal_pending(current)) {
		// 可以被信号 SIGINT，SIGQUIT 唤醒
		pr_info("hung_task_thread:%d, Received signals\n",
			current->pid);
	}

	if (ret == 0) {
		// 超时，被定时器唤醒
		pr_info("hung_task_thread:%d waked by timeout\n", current->pid);
	} else {
		// 被 rmmod 唤醒
		pr_info("hung_task_thread:%d, ret:%lld, waked by other\n",
			current->pid, ret);
	}
#elif (SCHED_TYPE == SCHED_NORMAL)
	// 会一直睡眠，直到被唤醒
	schedule();

	if (signal_pending(current)) {
		// 可以被信号 SIGINT，SIGQUIT 唤醒
		pr_info("hung_task_thread:%d,  Received signals\n",
			current->pid);
	}
	// 被 rmmod 唤醒
	pr_info("hung_task_thread:%d, waked by rmmod\n", current->pid);

#endif

	// 模拟工作
	msleep(1000);

	pr_info("hung_task_thread:%d, Exit\n", current->pid);
	return 0;
}

static int32_t __init __cw_hung_task_test_init(void)
{
	int32_t ret = 0;
	__cw_hung_task_thread = kthread_run(__cw_hung_task_thread_func, NULL,
					    "cw_hung_task_thread");
	if (IS_ERR(__cw_hung_task_thread)) {
		ret = PTR_ERR(__cw_hung_task_thread);
		pr_err("Failed to start 'cw_hung_task_thread' thread, ret:%d\n",
		       ret);
		return ret;
	}
	pr_info("hung task ptr is 0x%pK (actual=0x%px)\n",
		__cw_hung_task_thread, __cw_hung_task_thread);
	// 增加引用计数
	get_task_struct(__cw_hung_task_thread);
	pr_info("Hello, hung task test module\n");
	return 0;
}

static void __exit __cw_hung_task_test_exit(void)
{
	if (!IS_ERR(__cw_hung_task_thread)) {
		pr_info("hung task thread __state:0x%0x\n",
			__cw_hung_task_thread->__state);
		if (__cw_hung_task_thread->__state & TASK_RUNNING ||
		    __cw_hung_task_thread->__state & TASK_UNINTERRUPTIBLE) {
			// 在调用 kthread_stop 之前，一定要检查需要被终止的线程是否还在运行。如果 kthread_stop 调用了一个未运行的线程，可能会导致程序崩溃
			kthread_stop(__cw_hung_task_thread);
		}
	}

	pr_info("Goodbye, hung task test module\n");
}

module_init(__cw_hung_task_test_init);
module_exit(__cw_hung_task_test_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("hung task Example");
MODULE_VERSION("0.1");
