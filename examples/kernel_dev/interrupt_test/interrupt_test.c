/*
 * @Author: CALM.WU
 * @Date: 2024-04-19 17:27:59
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2024-04-19 17:27:59
 */

#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/uaccess.h> // for copy_to/from_user()