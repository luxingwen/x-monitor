/*
 * @Author: calmwu
 * @Date: 2024-06-15 11:23:28
 * @Last Modified by: calmwu
 * @Last Modified time: 2024-06-15 13:18:22
 */

#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>

// 如果编译没有代入版本信息
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

#define MODULE_TAG "CW_IOCTL_TEST"
#define CW_IOCTL_DEV_MAJOR 0
#define CW_IOCTL_NR_DEVS 2

static int32_t __cw_ioctl_dev_major = CW_IOCTL_DEV_MAJOR;
static int32_t __cw_ioctl_dev_minor = 0;
static int32_t __cw_ioctl_nr_devs = CW_IOCTL_NR_DEVS;

// 定义模块参数
module_param(__cw_ioctl_dev_major, int32_t, S_IRUGO);
module_param(__cw_ioctl_dev_minor, int32_t, S_IRUGO);
module_param(__cw_ioctl_nr_devs, int32_t, S_IRUGO);

static int32_t __ioctl_dev_open(struct inode *inode, struct file *filp);
static int32_t __ioctl_dev_release(struct inode *inode, struct file *filp);
static ssize_t __ioctl_dev_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos);
static ssize_t __ioctl_dev_write(struct file *filp, const char __user *buf,
				 size_t count, loff_t *f_pos);
static long __ioctl_dev_ioctl(struct file *filp, unsigned int cmd,
			      unsigned long arg);

static int32_t __ioctl_dev_open(struct inode *inode, struct file *filp)
{
	pr_info(CW_IOCTL_TEST " device file opened\n");
	return 0;
}

static int32_t __init cw_ioctl_test_init(void)
{
	return 0;
}

static void __exit cw_ioctl_test_exit(void)
{
}

module_init(cw_ioctl_test_init);
module_exit(cw_ioctl_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("ioctl test");
MODULE_VERSION("0.1");