/*
 * @Author: CALM.WU
 * @Date: 2022-11-17 11:53:49
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-17 16:41:52
 */

#pragma once

#include "../sock_info.h"

extern int32_t           init_sock_info_mgr();
extern struct sock_info *alloc_sock_info();
extern int32_t           add_sock_info(struct sock_info *sock_info);
extern struct sock_info *find_sock_info(uint32_t ino);
extern void              del_all_sock_info();
extern void              fini_sock_info_mgr();
extern int32_t           do_sock_info_collection();
