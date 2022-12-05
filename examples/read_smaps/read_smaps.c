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

#define BLOCK_BUF_SIZE 1024

static const char *__smaps_file = "../examples/read_smaps/dbus.smaps";

static void test_read_write_smaps() {
    int32_t smaps_fd = open(__smaps_file, O_RDONLY);
    if (unlikely(-1 == smaps_fd)) {
        error("open %s failed", __smaps_file);
        return;
    }

    int32_t output_fd =
        open("../examples/read_smaps/output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    char    block_buf[BLOCK_BUF_SIZE];
    ssize_t rest_bytes = 0, read_bytes = 0;
    char *  cursor = NULL, *line_end = NULL;
    ssize_t line_index = 1, line_size = 0;

    while ((read_bytes = read(smaps_fd, block_buf + rest_bytes, BLOCK_BUF_SIZE - rest_bytes - 1))
           > 0) {
        rest_bytes += read_bytes;
        block_buf[rest_bytes] = '\0';

        debug("read_bytes: %ld, rest_bytes: %ld", read_bytes, rest_bytes);
        cursor = block_buf;

        while ((line_end = strchr(cursor, '\n')) != NULL) {
            line_size = line_end - cursor + 1;

            write(output_fd, cursor, line_size);

            *line_end = '\0';

            rest_bytes -= line_size;
            if (rest_bytes > 0) {
                cursor = line_end + 1;
            }
            debug("line %ld, line_size: %ld, rest_bytes %ld", line_index, line_size, rest_bytes);
            line_index++;
        }

        if (rest_bytes > 0) {
            debug("memove rest %ld bytes", rest_bytes);
            memmove(block_buf, cursor, rest_bytes);
        }
    }

    close(smaps_fd);
    close(output_fd);
}

int32_t main(int32_t argc, char **argv) {
    if (log_init("../examples/log.cfg", "read_smaps") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    if (unlikely(argc != 2)) {
        fatal("./read_smaps_test <pid>\n");
        return -1;
    }

    pid_t pid = str2int32_t(argv[1]);
    if (unlikely(0 == pid)) {
        fatal("./read_smaps_test <pid>\n");
        return -1;
    }

    // test_read_write_smaps();

    struct process_smaps_info psi;
    __builtin_memset(&psi, 0, sizeof(psi));

    get_mss_from_smaps(pid, &psi);

    debug("pid: %d, vmsize: %lu, rss: %lu, pss: %lu, uss: %lu", pid, psi.vmsize, psi.rss, psi.pss,
          psi.uss);

    log_fini();

    return 0;
}