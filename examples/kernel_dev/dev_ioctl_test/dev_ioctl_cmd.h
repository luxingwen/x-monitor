/*
 * @Author: CALM.WU
 * @Date: 2024-06-24 14:13:26
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-07-08 17:08:09
 */

#ifndef __CW_DEV_IOCTL_CMD_H
#define __CW_DEV_IOCTL_CMD_H

// 定义 ioctl 命令
#define CW_IOCTL_MAGIC 0xA7 // Document/ioctl/ioctl-number.txt

/*
 * The _IO{R|W}() macros can be summarized as follows:
_IO(type,nr)                  ioctl command with no argument
_IOR(type,nr,datatype)        ioctl command for reading data from the kernel/drv
_IOW(type,nr,datatype)        ioctl command for writing data to the kernel/drv
_IOWR(type,nr,datatype)       ioctl command for read/write transfers
*/

#define CW_IOCTL_RESET _IO(CW_IOCTL_MAGIC, 0)
#define CW_IOCTL_GET_LENS _IOR(CW_IOCTL_MAGIC, 1, int32_t)
#define CW_IOCTL_SET_LENS _IOW(CW_IOCTL_MAGIC, 2, int32_t)

#define CW_IOCTL_MAX_NRS 3

#endif /* __CW_DEV_IOCTL_CMD_H */
