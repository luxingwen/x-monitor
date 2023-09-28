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
#include "utils/x_ebpf.h"

static const int32_t __crycle_counts = 1000;

int test(int x) {
    int c = 10;
    return x * c;
}

int32_t main(int32_t argc, char *argv[]) {
    if (log_init("../examples/log.cfg", "stackunwind_cli") != 0) {
        fprintf(stderr, "log init failed\n");
        return EXIT_FAILURE;
    }

    debug("stack_unwind_cli pid:%d", getpid());

    int a, b, c, d;
    a = 10;
    b = 11;
    debug("hello test~, %d", a + b);

    while (1) {
        d = __crycle_counts;
        do {
            c = test(a + b);
        } while (d--);
        usleep(50 * 1000);
        // debug("hello test(a + b) = %d", c);
    }

    log_fini();

    return 0;
}