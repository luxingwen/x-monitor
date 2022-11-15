/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 15:06:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-15 16:42:59
 */

#pragma once

#include "common_def.h"

extern const char *sock_state_name(enum SOCK_STATE ss);

extern int32_t do_sock_diag();

extern struct sock_info *find_sock_info(uint32_t ino);