/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 11:00:09
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-08-22 15:03:42
 */

#pragma once

#include "utils/common.h"

extern int32_t cgroup_collector_routine_init();
extern void   *cgroup_collector_routine_start(void *arg);
extern void    cgroup_collector_routine_stop();