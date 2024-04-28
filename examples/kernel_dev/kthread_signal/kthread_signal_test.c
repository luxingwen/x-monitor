/*
 * @Author: calmwu
 * @Date: 2024-04-07 15:01:42
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-28 17:29:41
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/sched/signal.h>
#include <linux/timer.h> // for timer_list API
#include <linux/param.h> // for HZ
#include <linux/jiffies.h> // for jiffies

static struct task_struct *__cw_thread = NULL;
static int32_t __cw_is_signal_exited = 0;

static int32_t __cw_kthread_signal_handler(void *data)
{
	int64_t ret = 0;
	allow_signal(SIGINT);
	allow_signal(SIGTERM);
	allow_signal(SIGKILL);
	allow_signal(SIGUSR1);

	// 等待 1s
	mdelay(1000);

	for (;;) {
		if (kthread_should_stop()) {
			pr_info("cw_kthread_signal:%d Stopping...\n",
				current->pid);
			do_exit(999);
		}

		pr_info("cw_kthread_signal:%d, going to sleep 5s\n",
			current->pid);
		// 设置 task 可以被信号中断
		set_current_state(TASK_INTERRUPTIBLE);
		// 调度，等待唤醒或者超时，将毫秒转换为节拍数，这里是 5 秒
		ret = schedule_timeout(msecs_to_jiffies(5000));

		if (signal_pending(current)) {
			pr_info("cw_kthread_signal:%d, Signal received\n",
				current->pid);
			break;
		}

		if (ret == 0) {
			pr_info("cw_kthread_signal:%d, Wakened by the Timer\n",
				current->pid);
		} else {
			pr_info("cw_kthread_signal:%d, Wakened by the rmmod\n",
				current->pid);
			// kthread_should_stop will return false
		}
	}

	__cw_is_signal_exited = 1;
	return 0;
}

static int32_t __init __cw_kthread_signal_init(void)
{
	int32_t ret = 0;

	__cw_thread = kthread_run(__cw_kthread_signal_handler, NULL,
				  "cw_kthread_signal");
	if (IS_ERR(__cw_thread)) {
		ret = PTR_ERR(__cw_thread);
		pr_err("%s: Failed to start 'cw_kthread_signal' thread (%d)\n",
		       __func__, ret);
		return ret;
	} else {
		pr_info("cw_kthread_signal:%d started\n",
			task_pid_nr(__cw_thread));
	}
	pr_info("kthread_signal_test module init\n");
	return 0;
}

static void __exit __cw_kthread_signal_exit(void)
{
	int32_t ret = 0;
	pid_t pid = task_pid_nr(__cw_thread);

	if (!__cw_is_signal_exited && !IS_ERR(__cw_thread)) {
		// ret 为内核线程的返回码，也就是 do_exit 的参数 999
		ret = kthread_stop(__cw_thread);
		pr_info("cw_kthread_signal:%d stopped. ret: %d\n", pid, ret);
	}
	pr_info("kthread_signal_test module exit\n");
}

module_init(__cw_kthread_signal_init);
module_exit(__cw_kthread_signal_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("kthread signal Example");
MODULE_VERSION("0.1");