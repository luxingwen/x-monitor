/*
 * @Author: CALM.WU
 * @Date: 2024-04-10 14:34:01
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-10 15:34:10
 */
/*
 * /proc
 *  ...
 *  |---cw_procfs_test           <-- our proc directory
 *      |---dbg_level
 *      |---show_pgoff
 *      |---show_drvctx
 *      |---config
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/uaccess.h> // for copy_to/from_user()
#include <linux/slab.h> // for kmalloc()
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>

// 如果编译没有代入版本信息
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

#ifndef LINUX_VERSION_CODE
#error "LINUX_VERSION_CODE is undefined"
#else
// 打印 LINUX_VERSION_CODE 宏的值
#pragma message("LINUX_VERSION_CODE: " __stringify(LINUX_VERSION_CODE))
#endif

#ifndef KERNEL_VERSION
#error "KERNEL_VERSION is undefined"
#endif

#define PROCFS_TEST_DBG_LEVEL "dbg_level"
#define PROCFS_TEST_DBG_LEVEL_PERMS 0644
#define PROC_TEST_DIRNAME "cw_procfs_test"
#define KEY_SIZE 128

DEFINE_MUTEX(mtx);

static struct proc_dir_entry *__cw_procfs_dir = NULL;

struct cw_drv_ctx {
    int32_t tx, rx, word, power;
    int32_t debug_level;
    char key[KEY_SIZE];
};
static struct cw_drv_ctx *__cw_drv_ctx;

//--------------------------------------------------------
static struct cw_drv_ctx *__alloc_cw_drv_ctx() {
    struct cw_drv_ctx *dc = NULL;

    dc = kzalloc(sizeof(struct cw_drv_ctx), GFP_KERNEL);
    if (unlikey(!dc)) {
        pr_err("kzalloc struct cw_drv_ctx failed.\n");
        // 将错误吗封装成一个特殊的指针值，获取这个指针后，使用 IS_ERR
        // 来判断是不是一个错误指针，用 PTR_ERR 来获得错误码 #define
        // IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO),
        // 实际上内核地址是不可能大于 (unsigned long)-MAX_ERRNO 的
        return ERR_PTR(-ENOMEM);
    }

    return dc;
}

//--------------------------------------------------------
// for /proc/cw_procfs_test/dbg_level
static ssize_t __cw_write_dbg_level(struct file *f, const char __user *ubuf,
                                    size_t count, loff_t *off) {
    int32_t ret = count;
    return ret;
}

static int32_t __cw_show_dbg_level(struct seq_file *seq, void *data) {
    // 可以被信号中断，例如 CTRL+C，在用户交互的处理中，最好使用可中断的 mutex
    if (mutext_lock_interruptible(&mtx))
        // 被信号中断，返回 -ERESTARTSYS，调用方应该重新调用
        return -ERESTARTSYS;
    seq_printf(seq, "debug_level:%d\n", __cw_drv_ctx->debug_level);
    mutex_unlock(&mtx);
    return 0;
}

static int32_t __cw_open_dbg_level(struct inode *in, struct file *f) {
    return single_open(f, __cw_show_dbg_level, NULL);
}

// proc file operation 在内核 5.6.0 以上改为 struct proc_ops
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
static struct file_operations __cw_fops_dbg_level = {
    .owner = THIS_MODULE,
    .open = __cw_open_dbg_level,
    .read = NULL,
    .write = NULL,
    .llseek = seq_lseek,
    .release = single_release,
};
#else
static struct proc_fops __cw_fops_dbg_level = {
    .proc_open = NULL,
    .proc_read = NULL,
    .proc_write = NULL,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#endif

//--------------------------------------------------------
// 模块初始化，注销函数
static int32_t __init cw_procfs_test_init(void) {
    int32_t ret = 0;

    // 判读是否支持 procfs
    if (unlikely(!IS_ENABLED(CONFIG_PROC_FS))) {
        pr_warn("procfs unsupported! Aborting ...\n");
        return -EINVAL;
    }

    __cw_drv_ctx = __alloc_cw_drv_ctx();
    if (IS_ERR(__cw_drv_ctx)) {
        pr_err("device context alloc failed.\n");
        ret = PTR_ERR(__cw_drv_ctx);
        return ret;
    }

    // 创建/proc 下的 parent dir
    __cw_procfs_dir = proc_mkdir(PROC_TEST_DIRNAME, NULL);
    if (unlikely(!__cw_procfs_dir)) {
        pr_err("proc_mkdir: '/proc/%s' failed.\n", PROC_TEST_DIRNAME);
        // 设置返回码
        ret = -ENOMEM;
        goto init_fail_1;
    }
    pr_info("procfs dir: '/proc/%s' created.\n", PROC_TEST_DIRNAME);

    // 创建 dbg_level entry
    if (!proc_create(PROCFS_TEST_DBG_LEVEL, PROCFS_TEST_DBG_LEVEL_PERMS,
                     __cw_procfs_dir, &__cw_fops_dbg_level)) {
        pr_err("proc_create %s failed.\n");
        ret = -ENOMEM;
        goto init_fail_2;
    }
    pr_info("procfs file: '/proc/%s/%s' created.\n", PROC_TEST_DIRNAME,
            PROCFS_TEST_DBG_LEVEL);

    pr_info("cw procfs test init\n");
    return 0;

init_fail_2:
    // 删除/proc/cw_procfs_test 目录
    remove_proc_subtree(PROC_TEST_DIRNAME, NULL);
init_fail_1:
    kzfree(__cw_drv_ctx);

    return ret;
}

static void __exit cw_procfs_test_exit(void) {
    // 上次/proc/cw_procfs_test 目录树
    remove_proc_subtree(PROC_TEST_DIRNAME, NULL);
    destroy_mutex(&mtx);
    if (!IS_ERR(__cw_drv_ctx))
        kzfree(__cw_drv_ctx);
    pr_info("cw procfs test exit\n");
}

module_init(cw_procfs_test_init);
module_exit(cw_procfs_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("calmwu <wubo0067@hotmail.com>");
MODULE_DESCRIPTION("ProcFS Example");
MODULE_VERSION("0.1");