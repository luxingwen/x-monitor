/*
 * @Author: CALM.WU
 * @Date: 2022-09-07 11:36:44
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 14:33:57
 */

#include "cgroups_subsystem.h"
#include "cgroups_def.h"

#include "utils/common.h"
#include "utils/log.h"
#include "utils/compiler.h"
#include "utils/strings.h"

#include "prometheus-client-c/prom.h"

void init_cgroup_obj_memory_metrics(struct xm_cgroup_obj *cg_obj) {
    debug("[PLUGIN_CGROUPS] init cgroup obj:'%s' subsys-memory metrics success.", cg_obj->cg_id);
}

void read_cgroup_obj_memory_metrics(struct xm_cgroup_obj *cg_obj) {
    debug("[PLUGIN_CGROUPS] read cgroup obj:'%s' subsys-memory metrics", cg_obj->cg_id);
}