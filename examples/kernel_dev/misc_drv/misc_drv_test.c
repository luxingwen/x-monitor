/*
 * @Author: CALM.WU
 * @Date: 2024-07-16 15:57:37
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-07-16 17:58:53
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
#include <linux/miscdevice.h>
#include <linux/major.h>
#include "../misc.h"

// 如果编译没有代入版本信息
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

#define MODULE_TAG "Module:[cw_misc_drv]"

static int32_t __open_cw_miscdev(struct inode *inode, struct file *filp)
{
    char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

    if (unlikely(!buf))
        return -ENOMEM;
    PRINT_CTX(); // displays process (or atomic) context info

    pr_info(MODULE_TAG " opening \"%s\" now; wrt open file: f_flags = 0x%x\n",
            file_path(filp, buf, PATH_MAX), filp->f_flags);

    kfree(buf);
    return nonseekable_open(inode, filp);
}

static ssize_t __read_cw_miscdev(struct file *filp, char __user *buf,
                                 size_t count, loff_t *f_pos)
{
    pr_info(MODULE_TAG " to read %zd bytes\n", count);
    return count;
}

static ssize_t __write_cw_miscdev(struct file *filp, const char __user *buf,
                                  size_t count, loff_t *f_pos)
{
    pr_info(MODULE_TAG "to write %zd bytes\n", count);
    return count;
}

static int32_t __release_cw_miscdev(struct inode *inode, struct file *filp)
{
    char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

    if (unlikely(!buf))
        return -ENOMEM;
    pr_info(MODULE_TAG " closing \"%s\"\n", file_path(filp, buf, PATH_MAX));
    kfree(buf);

    return 0;
}

static struct file_operations __cw_misc_fops = {
    .open = __open_cw_miscdev,
    .read = __read_cw_miscdev,
    .write = __write_cw_miscdev,
    .release = __release_cw_miscdev,
    .llseek = no_llseek,
    .owner = THIS_MODULE,
};

static struct miscdevice __cw_miscdev = {
    .minor = MISC_DYNAMIC_MINOR, // 动态分配次设备号
    .name = "cw_miscdev", // misc_register 时会在 /dev 下创建 /dev/cw_miscdev, /sys/class/misc and /sys/devices/virtual/misc
    .mode = 0666,
    .fops = &__cw_misc_fops,
};

static int32_t __init cw_miscdrv_init(void)
{
    int32_t ret;
    struct device *dev;

    ret = misc_register(&__cw_miscdev);
    if (ret < 0) {
        pr_err(MODULE_TAG " misc device register failed. ret: %d\n", ret);
        return ret;
    }

    // 获得注册的 device
    dev = __cw_miscdev.this_device;
    pr_info(MODULE_TAG
            " misc device[10:%d], node:[/dev/%s] register success.\n",
            __cw_miscdev.minor, __cw_miscdev.name);
    dev_info(dev, MODULE_TAG " minor:%d", __cw_miscdev.minor);
    pr_info(MODULE_TAG " hello\n");
    return 0;
}

static void __exit cw_miscdrv_exit(void)
{
    misc_deregister(&__cw_miscdev);
    pr_info(MODULE_TAG " byte\n");
}

module_init(cw_miscdrv_init);
module_exit(cw_miscdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("miscdrv test");
MODULE_VERSION("0.1");
