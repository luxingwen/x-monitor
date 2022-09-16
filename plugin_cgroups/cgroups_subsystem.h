/*
 * @Author: CALM.WU
 * @Date: 2022-09-07 11:04:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 15:00:39
 */

#pragma once

#include <stdint.h>

struct xm_cgroup_obj;
struct plugin_cgroups_ctx;

#define DEF_CGROUP_SUBSYSTEM_FUNC(name)                                         \
    extern void init_cgroup_obj_##name##_metrics(struct xm_cgroup_obj *cg_obj); \
    extern void collect_cgroup_obj_##name##_metrics(struct xm_cgroup_obj *cg_obj);

DEF_CGROUP_SUBSYSTEM_FUNC(cpuacct)

DEF_CGROUP_SUBSYSTEM_FUNC(memory)

DEF_CGROUP_SUBSYSTEM_FUNC(blkio)

extern int32_t init_cgroup_memory_pressure_listener(struct xm_cgroup_obj *cg_obj,
                                                    const char           *cg_memory_base_path,
                                                    const char           *mem_pressure_level_str);
