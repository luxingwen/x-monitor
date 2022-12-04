/*
 * @Author: CALM.WU
 * @Date: 2022-12-04 11:49:46
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-04 11:53:43
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/strings.h"

int32_t main() {
    if (log_init("../examples/log.cfg", "read_smaps") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    log_fini();

    return 0;
}