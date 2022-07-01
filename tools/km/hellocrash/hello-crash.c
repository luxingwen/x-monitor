/*
 * @Author: calmwu
 * @Date: 2022-06-30 17:17:01
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-06-30 17:18:35
 */

#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/init.h>   /* Needed for the macros */

#define AUTHOR "calm.wu <wubo0067@hotmail.com>"
#define DESC "Cause kernel crash"

char *p = NULL;

static int __init hello_init(void) {
    printk(KERN_ALERT "Hello, world 1.\n");
    *p = 1;
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_ALERT "Goodbye, world 1.\n");
}

module_init(hello_init);
module_exit(hello_exit);

/*
 * Get rid of taint message by declaring code as GPL.
 */
MODULE_LICENSE("GPL");

/*
 * Or with defines, like this:
 */
MODULE_AUTHOR(AUTHOR);    /* Who wrote this module? */
MODULE_DESCRIPTION(DESC); /* What does this module do */
