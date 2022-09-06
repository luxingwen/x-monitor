/*
 * @Author: calmwu
 * @Date: 2022-08-31 20:53:10
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 22:27:30
 */

#pragma once

#include "cgroups_def.h"

extern int32_t cgroups_mgr_init(struct plugin_cgroups_ctx *ctx);
extern void    cgroups_mgr_start_discovery();
extern void    cgroups_mgr_collect_metrics();
extern void    cgroups_mgr_fini();
