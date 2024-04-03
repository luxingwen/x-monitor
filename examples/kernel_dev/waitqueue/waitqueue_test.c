/*
 * @Author: CALM.WU
 * @Date: 2024-04-01 15:25:26
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-01 18:22:00
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h> // for copy_to/from_user()
#include <linux/kthread.h>
#include <linux/wait.h> // for wait_queue
#include <linux/err.h>
#include <linux/slab.h> // for kmalloc()

#ifndef INIT_BY_DYN
static DECLARE_WAIT_QUEUE_HEAD(wq);
#else
static wait_queue_head_t wq;
#endif

static dev_t __cw_dev = 0;
static struct cdev __cw_cdev;
static struct class *__cw_cdev_class;
static int32_t __cw_wait_queue_flag = 0;

static int32_t __cw_cdev_open(struct inode *inode, struct file *filp);
static int32_t __cw_cdev_release(struct inode *inode, struct file *filp);
static ssize_t __cw_cdev_read(struct file *filp, char __user *buf, size_t lbuf,
                              loff_t *ppos);
static ssize_t __cw_cdev_write(struct file *filp, const char __user *buf,
                               size_t len, loff_t *f_pos);

// File operation structure
static struct file_operations __cw_cdev_fops = {
    .owner = THIS_MODULE,
    .read = __cw_cdev_read,
    .write = __cw_cdev_write,
    .open = __cw_cdev_open,
    .release = __cw_cdev_release,
};

static int32_t __cw_cdev_open(struct inode *inode, struct file *filp) {
    pr_info("Device '/dev/cw_waitqueue_test' Opened...!!!\n");
    return 0;
}

static int32_t __cw_cdev_release(struct inode *inode, struct file *filp) {
    pr_info("Device '/dev/cw_waitqueue_test' Closed...!!!\n");
    return 0;
}

static ssize_t __cw_cdev_read(struct file *filp, char __user *buf, size_t lbuf,
                              loff_t *ppos) {
    pr_info("Read from Device '/dev/cw_waitqueue_test'\n");
    __cw_wait_queue_flag = 1;
    return 0;
}

static ssize_t __cw_cdev_write(struct file *filp, const char __user *buf,
                               size_t len, loff_t *f_pos) {
    pr_info("Write to Device '/dev/cw_waitqueue_test'\n");
    return len;
}

static int __init cw_waitqueue_test_init(void) {
    pr_info("cw_waitqueue_test_init\n");

    // Allocate a major number dynamically
    // device name = "cw_waitqueue_test"
    if (unlikely(alloc_chrdev_region(&__cw_dev, 0, 1, "cw_waitqueue_test")
                 < 0)) {
        pr_err("alloc_chrdev_region failed\n");
        return -1;
    }
    pr_info("major: %d, minor: %d\n", MAJOR(__cw_dev), MINOR(__cw_dev));
    // Initialize the cdev structure
    cdev_init(&__cw_cdev, &__cw_cdev_fops);

    // Add character device to the system
    if (unlikely(cdev_add(&__cw_cdev, __cw_dev, 1) < 0)) {
        pr_err("cdev_add failed\n");
        goto un_class;
    }

    // creating struct class
    __cw_cdev_class = class_create(THIS_MODULE, "cw_waitqueue_test");
    if (unlikely(IS_ERR(__cw_cdev_class))) {
        pr_err("class_create failed\n");
        goto un_class;
    }

    // creating device
    if (unlikely(IS_ERR(device_create(__cw_cdev_class, NULL, __cw_dev, NULL,
                                      "cw_waitqueue_test")))) {
        pr_err("device_create failed\n");
        goto un_device;
    }

    // initialize waitqueue
#ifndef INIT_BY_DNY
    pr_info("use static waitqueue initialization\n");
#else
    pr_info("use dynamic waitqueue initialization\n");
    init_waitqueue_head(&wq);
#endif

    // will create kernel thread

    pr_info("waitqueue test device '/dev/cw_waitqueue_test' created\n");
    return 0;

un_device:
    class_destroy(__cw_cdev_class);
un_class:
    // Unregister the device
    unregister_chrdev_region(__cw_dev, 1);
    return 0;
}

static void __exit cw_waitqueue_test_exit(void) {
    __cw_wait_queue_flag = 2;
    // wake up the kernel thread

    device_destroy(__cw_cdev_class, __cw_dev);
    class_destroy(__cw_cdev_class);
    cdev_del(&__cw_cdev);
    unregister_chrdev_region(__cw_dev, 1);
    pr_info("waitqueue test device '/dev/cw_waitqueue_test' removed\n");
}

module_init(cw_waitqueue_test_init);
module_exit(cw_waitqueue_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("Waitqueue Example");
MODULE_VERSION("0.1");