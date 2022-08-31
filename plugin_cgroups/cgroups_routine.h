/*
 * @Author: calmwu
 * @Date: 2022-08-31 20:53:10
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 21:27:44
 */

#pragma once

#include <stdint.h>

extern int32_t start_cgroups_update_routine(void *args);

extern void stop_cgroups_update_routine();

extern void notify_cgroups_update_now();