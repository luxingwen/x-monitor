/*
 * @Author: CALM.WU
 * @Date: 2023-04-01 12:57:31
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2023-04-01 12:57:31
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUF_LEN 1024

static int Major;
static char msg[BUF_LEN];
static int msg_len;
static int Device_Open = 0;

static int device_open(struct inode *inode, struct file *file) {
    if (Device_Open)
        return -EBUSY;

    Device_Open++;
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    Device_Open--;
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length,
                           loff_t *offset) {
    int bytes_read = 0;

    if (*offset >= msg_len)
        return 0;

    while (length && (*offset < msg_len)) {
        put_user(msg[*offset], buffer++);
        length--;
        (*offset)++;
        bytes_read++;
    }

    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char *buffer,
                            size_t length, loff_t *offset) {
    int i;

    for (i = 0; i < length && i < BUF_LEN; i++)
        get_user(msg[i], buffer + i);

    msg_len = i;
    return i;
}

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init mychardev_init(void) {
    Major = register_chrdev(0, DEVICE_NAME, &fops);
    if (Major < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", Major);
        return Major;
    }

    printk(KERN_INFO "Registered char device with major number %d\n", Major);
    return 0;
}

static void __exit mychardev_exit(void) {
    unregister_chrdev(Major, DEVICE_NAME);
    printk(KERN_INFO "Unregistered char device\n");
}

module_init(mychardev_init);
module_exit(mychardev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple char device driver");
