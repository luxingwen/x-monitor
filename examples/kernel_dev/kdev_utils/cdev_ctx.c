/*
 * @Author: calmwu
 * @Date: 2024-06-15 22:16:33
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-06-24 17:29:25
 */

#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/compiler.h>

#include "cdev_ctx.h"

/**
 * @brief Creates character devices for a module.
 *
 * This function creates character devices for a module using the provided context.
 *
 * @param ctx The context containing the necessary information for creating the character devices.
 * @return 0 on success, or a negative error code on failure.
 */
int32_t module_create_cdevs(struct module_cdev_crt_ctx *ctx)
{
	int32_t ret = 0;
	uint32_t i, j, k = 0;
	dev_t dev;
	struct device *device;

	if (unlikely(!ctx)) {
		pr_err("Invalid context\n");
		return -EINVAL;
	}

	if (unlikely(0 == ctx->fops)) {
		pr_err("Invalid file operations\n");
		return -EINVAL;
	}

	if (unlikely(0 == ctx->cdev_driver_name || 0 == ctx->cdev_cls_name ||
		     0 == ctx->cdev_name_fmt)) {
		pr_err("Invalid driver name, class name or device name format\n");
		return -EINVAL;
	}

	if (ctx->major == 0) {
		// 使用动态分配的方式获取主设备号
		ret = alloc_chrdev_region(&dev, ctx->base_minor, ctx->count,
					  ctx->cdev_driver_name);
		if (ret < 0) {
			pr_err("%s: alloc_chrdev_region failed\n",
			       ctx->cdev_driver_name);
			return ret;
		}
		ctx->major = MAJOR(dev);
	} else {
		// 使用指定的主设备号
		dev = MKDEV(ctx->major, ctx->base_minor);
		ret = register_chrdev_region(dev, ctx->count,
					     ctx->cdev_driver_name);
		if (ret < 0) {
			pr_err("%s: register_chrdev_region failed\n",
			       ctx->cdev_driver_name);
			return ret;
		}
	}
	pr_info("%s: dev major:%d, minor:%d\n", ctx->cdev_driver_name,
		ctx->major, MAJOR(dev));

	// create class, /sys/class
	ctx->cdev_cls = class_create(ctx->owner, ctx->cdev_cls_name);
	if (IS_ERR(ctx->cdev_cls)) {
		pr_err("%s: couldn't create %s class\n", ctx->cdev_driver_name,
		       ctx->cdev_cls_name);
		ret = PTR_ERR(ctx->cdev_cls);
		goto unreg_chrdev;
	}

	// create cdev array
	ctx->cdev_ary = (struct cdev *)kmalloc_array(
		ctx->count, sizeof(struct cdev), GFP_KERNEL | __GFP_ZERO);
	if (unlikely(!ctx->cdev_ary)) {
		ret = -ENOMEM;
		goto unreg_class;
	}

	// 添加设备
	for (i = 0; i < ctx->count; i++) {
		cdev_init(&ctx->cdev_ary[i], ctx->fops);
		ctx->cdev_ary[i].owner = ctx->owner;
		ret = cdev_add(&ctx->cdev_ary[i], dev + i, 1);
		if (ret < 0) {
			pr_err("%s: cdev_add %s%d failed\n",
			       ctx->cdev_driver_name, ctx->cdev_name_fmt, i);
			goto un_cdev_add;
		} else {
			j = i;
			pr_info("%s: cdev_add %s%d success\n",
				ctx->cdev_driver_name, ctx->cdev_name_fmt, i);
		}
	}
	j = ctx->count;

	// 创建设备 /dev
	for (i = 0; i < ctx->count; i++) {
		device = device_create(ctx->cdev_cls, NULL, dev + i,
				       ctx->dev_priv_data, ctx->cdev_name_fmt,
				       i);
		if (IS_ERR(device)) {
			pr_err("%s: device_create '%s'[%d] failed\n",
			       ctx->cdev_driver_name, ctx->cdev_name_fmt, i);
			ret = PTR_ERR(device);
			goto un_dev_crt;
		} else {
			k = j;
			pr_info("%s: device_create '%s'[%d] success\n",
				ctx->cdev_driver_name, ctx->cdev_name_fmt, i);
		}
	}

	pr_info("%s module_create_cdevs success\n", ctx->cdev_driver_name);
	return 0;

un_dev_crt:
	for (i = 0; i < k; i++) {
		device_destroy(ctx->cdev_cls, dev + i);
	}
un_cdev_add:
	for (i = 0; i < j; i++) {
		cdev_del(&ctx->cdev_ary[i]);
	}
	kfree(ctx->cdev_ary);
unreg_class:
	class_destroy(ctx->cdev_cls);
unreg_chrdev:
	unregister_chrdev_region(dev, ctx->count);
	return ret;
}

EXPORT_SYMBOL_GPL(module_create_cdevs);

/**
 * @brief Destroys the character devices associated with the given module context.
 *
 * This function is responsible for destroying the character devices that were created
 * using the module_cdev_crt_ctx structure. It should be called when the module is being
 * unloaded or when the character devices are no longer needed.
 *
 * @param ctx The module_cdev_crt_ctx structure containing the character devices to be destroyed.
 */
void module_destroy_cdevs(struct module_cdev_crt_ctx *ctx)
{
	uint32_t i;
	dev_t dev = MKDEV(ctx->major, ctx->base_minor);

	if (unlikely(!ctx)) {
		pr_err("Invalid context\n");
		return;
	}
	// 删除设备
	for (i = 0; i < ctx->count; i++) {
		device_destroy(ctx->cdev_cls, dev + i);
		pr_info("%s: device_destroy dev:[%d:%d]\n",
			ctx->cdev_driver_name, ctx->major, ctx->base_minor + i);
	}
	// 删除类
	class_destroy(ctx->cdev_cls);
	// 删除 cdev
	for (i = 0; i < ctx->count; i++) {
		cdev_del(&ctx->cdev_ary[i]);
	}
	// 释放 cdev 数组
	kfree(ctx->cdev_ary);
	// 释放设备号
	unregister_chrdev_region(dev, ctx->count);
	pr_info("%s module_destroy_cdevs success\n", ctx->cdev_driver_name);
}

EXPORT_SYMBOL_GPL(module_destroy_cdevs);
