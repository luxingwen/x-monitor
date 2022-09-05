/*
 * @Author: calmwu
 * @Date: 2022-08-31 20:53:10
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 22:27:30
 */

#pragma once

#include "cgroups_def.h"

extern void    cgroups_collect(struct plugin_cgroup_ctx *ctx);
extern int32_t cgroups_count();
extern void    cgroups_read_metrics(struct plugin_cgroup_ctx *ctx);
extern void    cgroups_free();