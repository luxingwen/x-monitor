/*
 * @Author: calmwu
 * @Date: 2024-06-15 22:16:08
 * @Last Modified by:   calmwu
 * @Last Modified time: 2024-06-15 22:16:08
 */

#ifndef __CW_KDEV_UILTS_CDEV_CTX_H
#define __CW_KDEV_UILTS_CDEV_CTX_H

struct file_operations;
struct module;
struct class;

// 字符设备的创建上下文
struct module_crt_cdev_ctx {
	int32_t major; // 主设备号，如果为 0，表明需要动态分配
	uint32_t base_minor; // 次设备号的起始值
	uint32_t count; // 次设备号的数量
	const char *cdev_driver_name; // 驱动名
	struct file_operations *fops; // 文件操作
	struct module *owner; // 模块
	const char *cdev_cls_name; // 设备类名
	const char *cdev_name_fmt; // 设备名格式
	void *dev_priv_data; // 设备私有数据

	struct class *cdev_cls; // 设备类
	struct cdev *cdev_ary; // 字符设备数组
};

int32_t module_create_cdevs(struct module_crt_cdev_ctx *ctx);
void module_destroy_cdevs(struct module_crt_cdev_ctx *ctx);

#endif /* __CW_KDEV_UILTS_CDEV_CTX_H */
