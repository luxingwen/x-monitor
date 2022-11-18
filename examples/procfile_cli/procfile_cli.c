/*
 * @Author: CALM.WU
 * @Date: 2021-12-06 11:13:59
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 11:37:31
 */

#include "utils/common.h"
#include "utils/log.h"
#include "utils/procfile.h"
#include "utils/strings.h"

const char *seps = " \t:";

int32_t main(int32_t argc, char **argv) {
    int32_t ret = 0;

    char    *proc_file = argv[1];
    uint32_t loop_secs = str2uint32_t(argv[2]);

    if (loop_secs == 0) {
        loop_secs = 10;
    }

    if (log_init("../examples/log.cfg", "procfile_cli") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    fprintf(stderr, "open proc file: %s", proc_file);

    struct proc_file *pf = procfile_open(proc_file, seps, PROCFILE_FLAG_DEFAULT);
    if (pf == NULL) {
        fprintf(stderr, "open proc file failed\n");
        return -1;
    }

    pf = procfile_readall(pf);
    size_t lines = procfile_lines(pf);
    debug("file '%s' has %zu lines", proc_file, lines);

    do {
        pf = procfile_readall(pf);
        procfile_print(pf);
        sleep(3);
    } while (--loop_secs);

    procfile_close(pf);

    log_fini();

    return ret;
}