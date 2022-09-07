/*
 * @Author: CALM.WU
 * @Date: 2022-09-07 11:04:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 15:00:39
 */

#pragma once

struct xm_cgroup_obj;

#define DEF_CGROUP_SUBSYSTEM_FUNC(name)                                         \
    extern void init_cgroup_obj_##name##_metrics(struct xm_cgroup_obj *cg_obj); \
    extern void read_cgroup_obj_##name##_metrics(struct xm_cgroup_obj *cg_obj);

DEF_CGROUP_SUBSYSTEM_FUNC(cpuacct)

DEF_CGROUP_SUBSYSTEM_FUNC(memory)

// DEF_CGROUP_SUBSYSTEM_FUNC(cpuset)

DEF_CGROUP_SUBSYSTEM_FUNC(blkio)

// DEF_CGROUP_SUBSYSTEM_FUNC(device)
