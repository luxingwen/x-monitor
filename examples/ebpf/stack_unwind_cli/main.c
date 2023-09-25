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

void func_d() {
    int msec = 1;
    debug("%s", "Hello world from D");
    usleep(10000 * msec);
}
void func_c() {
    debug("%s", "Hello from C");
    func_d();
}
void func_b() {
    debug("%s", "Hello from B");
    func_c();
}
void func_a() {
    debug("%s", "Hello from A");
    func_b();
}

int32_t main(int32_t argc, char *argv[]) {
    if (log_init("../examples/log.cfg", "stackunwind_cli") != 0) {
        fprintf(stderr, "log init failed\n");
        return EXIT_FAILURE;
    }

    debug("start unwind example");

    func_a();

    log_fini();

    return 0;
}