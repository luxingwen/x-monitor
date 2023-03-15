/*
 * @Author: CALM.WU
 * @Date: 2023-03-14 15:49:21
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-14 18:17:00
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/anon_inodes.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define AUTHOR "calm.wu <wubo0067@hotmail.com>"
#define DESC "Use vmalloc_user and mmap to access kernel var"
#define DEVICE_NAME "calmwu_mmap_dev"
#define ALLOC_SIZE PAGE_SIZE
#define CLASS_NAME "calmwu_mmap_mod_class"

static char *data;
static int major_num;
static struct class *module_class;
static struct cdev module_cdev;

static int my_dev_open(struct inode *inode, struct file *filp) {
    // 使用 vmalloc_user 分配内存
    // const gfp_t flags = __GFP_NOWARN | __GFP_ZERO | __GFP_ACCOUNT;

    // data = vmalloc_user_node_flags(ALLOC_SIZE, 0,
    //                                GFP_KERNEL | __GFP_RETRY_MAYFAIL | flags);
    data = vmalloc_user(ALLOC_SIZE);
    if (!data) {
        pr_err("vmalloc_user_node_flags failed");
        return -ENOMEM;
    }
    strlcpy(data,
            "This is the greeting from the CALMWU.mmap.device, from the "
            "kernel.",
            ALLOC_SIZE);

    // 设置data为file的private-data
    filp->private_data = data;
    pr_info("%s module: opened\n", DEVICE_NAME);
    return 0;
}

static int my_dev_release(struct inode *inode, struct file *filp) {
    if (filp->private_data) {
        vfree(filp->private_data);
    }

    pr_info("%s module: release\n", DEVICE_NAME);
    return 0;
}

static int my_dev_mmap(struct file *filp, struct vm_area_struct *vma) {
    char *datap = filp->private_data;
    // pgoff_t pgoff = 1;

    // if (remap_vmalloc_range(vma, datap, vma->vm_pgoff + pgoff)) {
    //     return -EAGAIN;
    // }
    unsigned long size = vma->vm_end - vma->vm_start;
    if (size > ALLOC_SIZE) {
        pr_err("vma size: %lu larger than ALLOC_SIZE: %lu", size, ALLOC_SIZE);
        return -EINVAL;
    }

    if (remap_pfn_range(vma, vma->vm_start, vmalloc_to_pfn(datap), size,
                        vma->vm_page_prot)) {
        return -EAGAIN;
    }

    pr_info("%s module: mmap\n", DEVICE_NAME);
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_dev_open,
    .mmap = my_dev_mmap,
    .release = my_dev_release,
};

static int __init my_module_init(void) {
    printk(KERN_INFO "Loading %s module...\n", DEVICE_NAME);

    // 创建一个字符设备
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n",
               major_num);
        return major_num;
    }

    module_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(module_class)) {
        unregister_chrdev(major_num, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(module_class);
    }

    device_create(module_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
    cdev_init(&module_cdev, &fops);
    cdev_add(&module_cdev, MKDEV(major_num, 0), 1);

    printk(KERN_INFO "%s module: loaded\n", DEVICE_NAME);

    return 0;
}

static void __exit my_module_exit(void) {
    cdev_del(&module_cdev);
    device_destroy(module_class, MKDEV(major_num, 0));
    class_unregister(module_class);
    class_destroy(module_class);
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "%s module: unloaded\n", DEVICE_NAME);
}

module_init(my_module_init);
module_exit(my_module_exit);

/*
 * Get rid of taint message by declaring code as GPL.
 */
MODULE_LICENSE("GPL");

/*
 * Or with defines, like this:
 */
MODULE_AUTHOR(AUTHOR); /* Who wrote this module? */
MODULE_DESCRIPTION(DESC); /* What does this module do */
