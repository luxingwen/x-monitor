/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 15:06:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-17 16:39:25
 */

#pragma once

#include "sock_info.h"

extern const char       *sock_state_name(enum SOCK_STATE ss);
extern int32_t           init_proc_socks();
extern void              collect_socks_info();
extern void              fini_proc_socks();
extern struct sock_info *find_sock_info(uint32_t ino);