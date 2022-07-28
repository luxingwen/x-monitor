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

        // 得到系统的cpu数量
        int32_t node_cpu_count = get_system_cpus();
        proc_rds->stat_rdset.core_rdsets =
            (struct proc_cpu_rdset *)calloc(node_cpu_count, sizeof(struct proc_cpu_rdset));
        if (unlikely(!proc_rds->stat_rdset.core_rdsets)) {
            error("calloc struct proc_cpu_rdset object failed");
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
        free(proc_rds);
        proc_rds = NULL;
    }
    debug("fini proc_raw_data_set successed.");
}

// void set_proc_rdset_init_success() {
//     if (likely(proc_rds && !proc_rds->init_flag)) {
//         proc_rds->init_flag ^= 1;
//         debug("proc_raw_data_set init flag is ok.");
//     }
// }
