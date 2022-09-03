/*
 * @Author: calmwu
 * @Date: 2022-09-03 15:13:11
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-09-03 16:18:57
 */

#include "utils/common.h"
#include "utils/log.h"
#include "libmill/libmill.h"

int32_t sum = 0;

coroutine void worker(int32_t count, int32_t n) {
    int32_t i;
    for (i = 0; i < count; ++i) {
        sum += n;
    }
    yield();
}

int32_t main(int32_t argc, char *argv[]) {
    if (log_init("../cli/log.cfg", "libmill_test_1") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    go(worker(3, 7));
    go(worker(1, 11));
    go(worker(2, 5));

    msleep(now() + 100);

    debug("sum=%d", sum);

    log_fini();

    return 0;
}
