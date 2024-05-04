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

// Interrupt Request number
#define IRQ_NUM 11
#define IRQ_NUM_TO_IVT_NUM (IRQ_NUM + 32)

static dev_t __cw_dev = 0;
static struct cdev __cw_cdev;
static struct class *__cw_cdev_class;

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

	pr_info("Trigger irq\n");
	// 将 irq 号转换为 irq 描述符
	desc = irq_to_desc(IRQ_NUM);
	if (!desc) {
		return -EINVAL;
	}
	__this_cpu_write(vector_irq[IRQ_NUM_TO_IVT_NUM], desc);
	asm("int $0x2B");
	return 0;
}

static struct file_operations __cw_irq_dev_fops = {
	.owner = THIS_MODULE,
	.read = __cw_irq_dev_read,
};

static int32_t __init __cw_irq_test_init(void)
{
	// Allocate a major number dynamically
	if (unlikely(alloc_chrdev_region(&__cw_dev, 0, 1, "cw_irq_test") < 0)) {
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
	__cw_cdev_class = class_create(THIS_MODULE, "cw_irq_test");
	if (unlikely(IS_ERR(__cw_cdev_class))) {
		pr_err("Cannot create the struct class\n");
		goto un_class;
	}

	// creating device
	if (unlikely(IS_ERR(device_create(__cw_cdev_class, NULL, __cw_dev, NULL,
					  "cw_irq_test")))) {
		pr_err("Cannot create the Device\n");
		goto un_device;
	}

	//注册一个 IRQ
	if (request_irq(IRQ_NUM, __cw_irq_handler, IRQF_SHARED, "cw_irq_test",
			(void *)(__cw_irq_handler))) {
		pr_err("Cannot register IRQ\n");
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