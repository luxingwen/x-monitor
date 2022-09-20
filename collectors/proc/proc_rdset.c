/*
 * @Author: CALM.WU
 * @Date: 2022-07-28 14:57:32
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-28 17:22:32
 */

#include "proc_rdset.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"

struct proc_rdset *proc_rds = NULL;

int32_t init_proc_rdset() {
    if (likely(!proc_rds)) {
        proc_rds = (struct proc_rdset *)calloc(1, sizeof(struct proc_rdset));
        if (unlikely(!proc_rds)) {
            error("calloc struct proc_rdset object failed");
            return -1;
        }

        proc_rds->stat_rdset.core_rdsets =
            (struct proc_cpu_rdset *)calloc(cpu_cores_num, sizeof(struct proc_cpu_rdset));

        proc_rds->schedstats =
            (struct proc_schedstat *)calloc(cpu_cores_num, sizeof(struct proc_schedstat));

        if (unlikely(!proc_rds->stat_rdset.core_rdsets || !proc_rds->schedstats)) {
            error("calloc struct proc_cpu_rdset or proc_schedstat object failed");
            goto INIT_FAILED;
        }
    }
    debug("init proc_raw_data_set successed.");
    return 0;

INIT_FAILED:
    fini_proc_rdset();
    return -1;
}

void fini_proc_rdset() {
    if (likely(proc_rds)) {
        if (proc_rds->stat_rdset.core_rdsets) {
            free(proc_rds->stat_rdset.core_rdsets);
            proc_rds->stat_rdset.core_rdsets = NULL;
        }
        if (proc_rds->schedstats) {
            free(proc_rds->schedstats);
            proc_rds->schedstats = NULL;
        }
        free(proc_rds);
        proc_rds = NULL;
    }
    debug("fini proc_raw_data_set successed.");
}
