/*
 * @Author: CALM.WU
 * @Date: 2023-04-01 12:57:31
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2023-04-01 12:57:31
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>

#define AUTHOR "calm.wu <wubo0067@hotmail.com>"
#define DESC                                                                \
    "Creates a read-only char device that says how many times you've read " \
    "from the dev file"
#define DEVICE_NAME "calmwu_chardev"
#define ALLOC_SIZE PAGE_SIZE
#define MSG_BUF_LEN 128

static int major_num;
static int device_is_open = 0;
static char msg[MSG_BUF_LEN] = { 0 };
static char *p_msg = msg;

static int device_open(struct inode *inode, struct file *filp) {
    static int counter = 0;

    if (device_is_open) {
        return -EBUSY;
    }

    device_is_open = 1;
    sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *inode, struct file *filp) {
    device_is_open--;
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length,
                           loff_t *offset) {
    int bytes_read = 0;

    if (*p_msg == 0) {
        // we're at the end of the message
        return 0;
    }

    while (length && *p_msg) {
        put_user(*(p_msg++), buffer++);
        length--;
        bytes_read++;
    }
    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char *buffer,
                            size_t length, loff_t *offset) {
    pr_err("Sorry, this operation isn't supported.\n");
    return -EINVAL;
}

static struct file_operations fops = { .read = device_read,
                                       .write = device_write,
                                       .open = device_open,
                                       .release = device_release };

static int __init __calmwu_chardev_module_init() {
    major_num = register_chrdev(0, DEVICE_NAME, NULL);

    if (major_num < 0) {
        pr_err("Registering char device failed with %d\n", major_num);
        return major_num;
    }

    pr_info("I was assigned major number %d, To talk to the driver, create ad "
            "dev file with\n",
            major_num);
    pr_info("'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, major_num);
    pr_info("Try various minor numbers. Try to cat and echo to the device "
            "file.\n");
    pr_info("Remove the device file and module when done.\n");
    return 0;
}

static void __exit __calmwu_chardev_module_exit() {
    int ret = unregister_chrdev(major_num, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Error in unregister_chrdev: %d\n", ret);
    }
}

module_init(__calmwu_chardev_module_init);
module_exit(__calmwu_chardev_module_exit);

/*
 * Get rid of taint message by declaring code as GPL.
 */
MODULE_LICENSE("GPL");

/*
 * Or with defines, like this:
 */
MODULE_AUTHOR(AUTHOR); /* Who wrote this module? */
MODULE_DESCRIPTION(DESC); /* What does this module do */