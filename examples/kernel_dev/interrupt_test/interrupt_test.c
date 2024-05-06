/*
 * @Author: CALM.WU
 * @Date: 2024-04-19 17:27:59
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2024-04-19 17:27:59
 */

#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/uaccess.h> // for copy_to/from_user()
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/hw_irq.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>

// 如果编译没有代入版本信息
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

// Interrupt Request number
#define IRQ_NUM 11
// 转换为中断向量表的下表
#define IRQ_NUM_TO_IVT_NUM (IRQ_NUM + 32)

#define CW_IRQ_DEV "cw_irq_trigger"
#define CW_IRQ_PROC_SWITCH "cw_irq_switch"

static dev_t __cw_dev = 0;
static struct cdev __cw_cdev;
static struct class *__cw_cdev_class;

static DEFINE_MUTEX(__mutex);

// 中断处理函数
static irqreturn_t __cw_irq_handler(int irq, void *dev_id)
{
	pr_info("irq %d triggered\n", irq);
	return IRQ_HANDLED;
}

// 设备文件的处理函数，用来触发中断
static ssize_t __cw_irq_dev_read(struct file *filp, char __user *buf,
				 size_t count, loff_t *f_pos)
{
	struct irq_desc *desc;

	pr_info("Trigger irq:%d\n", IRQ_NUM);
	// 将 irq 号转换为 irq 描述符
	desc = irq_to_desc(IRQ_NUM);
	if (!desc) {
		return -EINVAL;
	}
	__this_cpu_write(vector_irq[IRQ_NUM_TO_IVT_NUM], desc);
	asm("int $0x2B");
	return 0;
}

static ssize_t __cw_write_irq_switch(struct file *f, const char __user *ubuf,
				     size_t count, loff_t *off)
{
	char flag;

	if (mutex_lock_interruptible(&__mutex))
		return -ERESTARTSYS;

	// 读取用户态数据，put_user() 和 get_user() 用来从用户空间获取或者向用户空间发送单个值（例如一个 int，char，或者 long）
	get_user(flag, ubuf);
	if (flag == '0') {
		// 关闭中断
		pr_info("Disable irq:%d\n", IRQ_NUM);
		disable_irq(IRQ_NUM);
	} else {
		pr_info("Enable irq:%d\n", IRQ_NUM);
		enable_irq(IRQ_NUM);
	}

	mutex_unlock(&__mutex);
	return count;
}

// proc 文件，开关中断
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
static struct file_operations __cw_fops_irq_switch = {
	.owner = THIS_MODULE,
	.write = __cw_write_irq_switch,
};
#else
static struct proc_fops __cw_fops_irq_switch = {
	.proc_write = __cw_write_irq_switch,
};
#endif

// /dev/cw_irq_test文件的读取，触发中断
static struct file_operations __cw_irq_dev_fops = {
	.owner = THIS_MODULE,
	.read = __cw_irq_dev_read,
};

static int32_t __init __cw_irq_test_init(void)
{
	// Allocate a major number dynamically
	if (unlikely(alloc_chrdev_region(&__cw_dev, 0, 1, CW_IRQ_DEV) < 0)) {
		pr_err("Cannot allocate major number\n");
		return -1;
	}
	pr_info("major: %d, minor: %d\n", MAJOR(__cw_dev), MINOR(__cw_dev));
	// Initialize the cdev structure
	cdev_init(&__cw_cdev, &__cw_irq_dev_fops);

	// Add character device to the system
	if (unlikely(cdev_add(&__cw_cdev, __cw_dev, 1) < 0)) {
		pr_err("Cannot add the device to the system\n");
		goto un_class;
	}

	// creating struct class
	__cw_cdev_class = class_create(THIS_MODULE, CW_IRQ_DEV);
	if (unlikely(IS_ERR(__cw_cdev_class))) {
		pr_err("Cannot create the struct class\n");
		goto un_class;
	}

	// creating device
	if (unlikely(IS_ERR(device_create(__cw_cdev_class, NULL, __cw_dev, NULL,
					  CW_IRQ_DEV)))) {
		pr_err("Cannot create the Device\n");
		goto un_device;
	}

	//注册一个 IRQ
	if (request_irq(IRQ_NUM, __cw_irq_handler, IRQF_SHARED, CW_IRQ_DEV,
			(void *)(__cw_irq_handler))) {
		pr_err("Cannot register IRQ\n");
		goto un_irq;
	}
	// 创建中断控制文件/proc/cw_irq_swtch
	if (!proc_create(CW_IRQ_PROC_SWITCH, 0200, NULL,
			 &__cw_fops_irq_switch)) {
		pr_err("Cannot create proc file:'%s'\n", CW_IRQ_PROC_SWITCH);
		goto un_irq;
	}

	pr_info("Hello, irq test module\n");
	return 0;

un_irq:
	free_irq(IRQ_NUM, (void *)(__cw_irq_handler));

un_device:
	class_destroy(__cw_cdev_class);

un_class:
	unregister_chrdev_region(__cw_dev, 1);
	cdev_del(&__cw_cdev);
	return -1;
}

static void __exit __cw_irq_test_exit(void)
{
	free_irq(IRQ_NUM, (void *)(__cw_irq_handler));
	device_destroy(__cw_cdev_class, __cw_dev);
	remove_proc_entry(CW_IRQ_PROC_SWITCH, NULL);
	class_destroy(__cw_cdev_class);
	cdev_del(&__cw_cdev);
	unregister_chrdev_region(__cw_dev, 1);
	pr_info("Goodbye, irq test module\n");
}

module_init(__cw_irq_test_init);
module_exit(__cw_irq_test_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("delay function Example");
MODULE_VERSION("0.1");

/*
[Sat May  4 22:47:32 2024] interrupt_test:__cw_irq_test_init():65: major: 242, minor: 0
[Sat May  4 22:47:32 2024] interrupt_test:__cw_irq_test_init():96: Hello, irq test module
[Sat May  4 22:49:06 2024] interrupt_test:__cw_irq_dev_read():42: Trigger irq
[Sat May  4 22:49:06 2024] interrupt_test:__cw_irq_handler():32: irq 11 triggered
*/