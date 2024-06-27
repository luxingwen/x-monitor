/*
 * @Author: calmwu
 * @Date: 2024-06-15 11:23:28
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-06-24 16:52:14
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

#include <cdev_ctx.h>
#include "dev_ioctl_cmd.h"

#define MODULE_TAG "Module:[cw_ioctl_test]"
#define CW_IOCTL_DEV_MAJOR 0
#define CW_IOCTL_NR_DEVS 2

#define CW_DEV_NAME "cw_ioctl_dev"
#define CW_DEV_NAME_FMT "cw_ioctl_dev%d"

// 定义模块参数和变量
static int32_t __cw_ioctl_dev_major = CW_IOCTL_DEV_MAJOR;
module_param(__cw_ioctl_dev_major, int, S_IRUGO);

static int32_t __cw_ioctl_dev_minor = 0;
module_param(__cw_ioctl_dev_minor, int, S_IRUGO);

static int32_t __cw_ioctl_nr_devs = CW_IOCTL_NR_DEVS;
module_param(__cw_ioctl_nr_devs, int, S_IRUGO);

// 设备创建上下文
static struct cw_cdev_crt_ctx __cw_ioctl_dev_crt_ctx;

// 设备文件操作
static int32_t __ioctl_dev_open(struct inode *inode, struct file *filp);
static int32_t __ioctl_dev_release(struct inode *inode, struct file *filp);
static ssize_t __ioctl_dev_read(struct file *filp, char __user *buf,
                                size_t count, loff_t *f_pos);
static ssize_t __ioctl_dev_write(struct file *filp, const char __user *buf,
                                 size_t count, loff_t *f_pos);
static long __ioctl_dev_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg);

static struct file_operations __ioctl_dev_fops = {
    .owner = THIS_MODULE,
    .open = __ioctl_dev_open,
    .release = __ioctl_dev_release,
    .read = __ioctl_dev_read,
    .write = __ioctl_dev_write,
    .unlocked_ioctl = __ioctl_dev_ioctl,
};

static int32_t __ioctl_dev_open(struct inode *inode, struct file *filp)
{
    pr_info(MODULE_TAG " device file opened\n");
    return 0;
}

static int32_t __ioctl_dev_release(struct inode *inode, struct file *filp)
{
    pr_info(MODULE_TAG " device file released\n");
    return 0;
}

static ssize_t __ioctl_dev_read(struct file *filp, char __user *buf,
                                size_t count, loff_t *f_pos)
{
    pr_info(MODULE_TAG " read device file\n");
    return 0;
}

static ssize_t __ioctl_dev_write(struct file *filp, const char __user *buf,
                                 size_t count, loff_t *f_pos)
{
    pr_info(MODULE_TAG " write device file\n");
    return 0;
}

static long __ioctl_dev_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg)
{
    pr_info(MODULE_TAG " ioctl device file\n");
    return 0;
}

static int32_t __init cw_ioctl_test_init(void)
{
    int32_t ret = 0;

    __cw_ioctl_dev_crt_ctx.major = __cw_ioctl_dev_major;
    __cw_ioctl_dev_crt_ctx.base_minor = __cw_ioctl_dev_minor;
    __cw_ioctl_dev_crt_ctx.count = __cw_ioctl_nr_devs; // 设备数量
    __cw_ioctl_dev_crt_ctx.name = CW_DEV_NAME;
    __cw_ioctl_dev_crt_ctx.fops = &__ioctl_dev_fops;
    __cw_ioctl_dev_crt_ctx.name_fmt = CW_DEV_NAME_FMT;
    __cw_ioctl_dev_crt_ctx.dev_priv_data = NULL;

    ret = module_create_cdevs(&__cw_ioctl_dev_crt_ctx);
    if (unlikely(0 != ret)) {
        pr_err(MODULE_TAG " module_create_cdevs failed\n");
        return ret;
    }
    pr_info(MODULE_TAG " init successfully!\n");
    return 0;
}

static void __exit cw_ioctl_test_exit(void)
{
    module_destroy_cdevs(&__cw_ioctl_dev_crt_ctx);
    pr_info(MODULE_TAG " bye!\n");
}

module_init(cw_ioctl_test_init);
module_exit(cw_ioctl_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("ioctl test");
MODULE_VERSION("0.1");