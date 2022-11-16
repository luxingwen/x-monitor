/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 15:06:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-15 16:42:59
 */

#pragma once

#include "common_def.h"

extern const char *sock_state_name(enum SOCK_STATE ss);

// 初始化套接字诊断收集
extern int32_t init_socks_diag();

// 收集套接字诊断信息
extern int32_t collect_socks_diag();

// 清空套接字诊断信息
extern void fini_socks_diag();

// 查询套接字诊断信息
// ino：socket inode number
extern struct sock_info *find_sock_info(uint32_t ino);