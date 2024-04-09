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
#include <linux/pid.h>

#ifndef INIT_BY_DYN
static DECLARE_WAIT_QUEUE_HEAD(__cw_wq);
#else
static wait_queue_head_t __cw_wq;
#endif

static dev_t __cw_dev = 0;
static struct cdev __cw_cdev;
static struct class *__cw_cdev_class;

static int32_t __cw_wait_queue_flag = 0;
static uint32_t __cw_read_count = 0;
static struct task_struct *__cw_wait_thread;

#define DEV_BUF_SIZE 1024
static char *__cw_dev_buf;

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

static int32_t __cw_wait_thread_func(void *unused) {
    for (;;) {
        if (kthread_should_stop()) {
            // 但是 kthread_stop 不会自行终止线程，它会等待线程调用 do_exit
            // 或自行终止。kthread_stop 只确保如果线程调用函数
            // kthread_should_stop，则 kthread_should_stop 返回 true。
            pr_info("cw_wait_thread:%d Stopping...\n", current->pid);
            do_exit(0);
        }
        pr_info("cw_wait_thread:%d, Waiting for Event...\n", current->pid);
        wait_event_interruptible(__cw_wq, __cw_wait_queue_flag != 0);
        if (__cw_wait_queue_flag == 2) {
            pr_info("Event came from Exit function\n");
        } else {
            pr_info("cw_wait_thread:%d, Event came from Read function - %d\n",
                    current->pid, ++__cw_read_count);
            // 继续等待
            __cw_wait_queue_flag = 0;
        }
    }
    // 结束当前正在执行的线程，释放占用的 CPU 资源，参数是 error_code
    do_exit(0);
    return 0;
}

static int32_t __cw_cdev_open(struct inode *inode, struct file *filp) {
    pr_info("Device file '/dev/cw_waitqueue_test' Opened...!!!\n");
    return 0;
}

static int32_t __cw_cdev_release(struct inode *inode, struct file *filp) {
    pr_info("Device file '/dev/cw_waitqueue_test' Closed...!!!\n");
    return 0;
}

static ssize_t __cw_cdev_read(struct file *filp, char __user *buf, size_t lbuf,
                              loff_t *ppos) {
    ssize_t read_len = DEV_BUF_SIZE - 1;
    // 将数据从内核空间拷贝到用户空间
    if (copy_to_user(buf, __cw_dev_buf, read_len)) {
        pr_err("Device file '/dev/cw_waitqueue_test' read failed\n");
    } else {
        pr_info("Device file '/dev/cw_waitqueue_test' read Done\n");
    }

    __cw_wait_queue_flag = 1;
    // 唤醒
    wake_up_interruptible(&__cw_wq);
    return read_len;
}

static ssize_t __cw_cdev_write(struct file *filp, const char __user *buf,
                               size_t len, loff_t *f_pos) {
    memset(__cw_dev_buf, 0, DEV_BUF_SIZE);
    if (copy_from_user(__cw_dev_buf, buf, len)) {
        pr_err("Device file '/dev/cw_waitqueue_test' write failed\n");
    } else {
        pr_info("Device file '/dev/cw_waitqueue_test' write Done\n");
    }
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

    // allocate memory for device buffer
    // 创建小于 1 个 page 的内存，而且物理地址是连续的
    // 中断中分配内存一般用 GFP_ATOMIC，内核自己使用的内存一般用
    // GFP_KERNEL，为用户空间分配内存一般用 GFP_HIGHUSER_MOVABLE
    if ((__cw_dev_buf = kmalloc(DEV_BUF_SIZE, GFP_KERNEL)) == 0) {
        pr_err("Cannot allocate memory in kernel\n");
        goto un_device;
    }

    strncpy(__cw_dev_buf, "Hello from kernel world\n", (DEV_BUF_SIZE - 1));

    // initialize waitqueue
#ifndef INIT_BY_DNY
    pr_info("use static waitqueue initialization\n");
#else
    pr_info("use dynamic waitqueue initialization\n");
    init_waitqueue_head(&wq);
#endif

    // will create kernel thread
    __cw_wait_thread =
        kthread_create(__cw_wait_thread_func, NULL, "cw_wait_thread");
    if (!IS_ERR(__cw_wait_thread)) {
        // 新创建的线程是不会立即执行的，需要唤醒它
        wake_up_process(__cw_wait_thread);
        pr_info("cw_wait_thread: %d created\n", __cw_wait_thread->pid);
    } else {
        pr_err("cw_wait_thread create failed\n");
    }

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
    pid_t pid = 0;
    int32_t ret = 0;

    kfree(__cw_dev_buf);

    __cw_wait_queue_flag = 2;
    pid = task_pid_nr(__cw_wait_thread);
    // wake up the kernel thread
    wake_up_interruptible(&__cw_wq);
    // ret = 0 表示成功
    ret = kthread_stop(__cw_wait_thread);
    if (!ret) {
        pr_info("cw_wait_thread:%d stopped\n", pid);
    }

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