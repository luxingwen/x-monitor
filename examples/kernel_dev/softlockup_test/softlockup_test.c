/*
 * @Author: CALM.WU
 * @Date: 2024-04-22 11:30:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-22 14:33:58
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h> // for kernel thread

static int32_t __init __cw_softlockup_test_init(void) {
    return 0;
}