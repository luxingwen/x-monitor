/*
 * @Author: CALM.WU
 * @Date: 2023-09-25 10:14:14
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-25 10:43:18
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/consts.h"
#include "utils/strings.h"
#include "utils/clocks.h"
#include "utils/x_ebpf.h"

static const int32_t __crycle_counts = 1000;

int32_t test_1(int32_t x) {
    int32_t c = 12;
    int32_t d = 13;
    return x + c + d;
}

int32_t test(int32_t x) {
    int32_t a = 10;
    int32_t b = 11;
    return test_1(x + a + b);
}

int32_t main(int32_t argc, char *argv[]) {
    if (log_init("../examples/log.cfg", "stackunwind_cli") != 0) {
        fprintf(stderr, "log init failed\n");
        return EXIT_FAILURE;
    }

    srand(now_realtime_sec());

    int32_t r = rand();
    int32_t result = test(r);
    debug("test(%d)=%d", r, result);

    log_fini();

    return 0;
}