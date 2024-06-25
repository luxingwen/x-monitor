/*
 * @Author: CALM.WU
 * @Date: 2024-06-24 14:13:26
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-06-24 15:22:17
 */

#ifndef __CW_DEV_IOCTL_CMD_H
#define __CW_DEV_IOCTL_CMD_H

// 定义 ioctl 命令
#define CW_IOCTL_MAGIC 0xA7 // Document/ioctl/ioctl-number.txt

#define CW_IOCTL_WR_VALUE _IOW(CW_IOCTL_MAGIC, 0, int32_t *)
#define CW_IOCTL_RD_VALUE _IOR(CW_IOCTL_MAGIC, 1, int32_t *)

#define CW_IOCTL_MAX_NRS 2

#endif /* __CW_DEV_IOCTL_CMD_H */
