/*
 * @Author: CALM.WU
 * @Date: 2023-03-14 15:49:51
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2023-03-14 15:49:51
 */

#include "utils/common.h"
#include "utils/log.h"
#include "utils/strings.h"
#include "utils/compiler.h"
#include "utils/os.h"

int32_t main(int32_t argc, char **argv) {
    int32_t fd;
    void *mapped;
    const size_t mapping_size = sysconf(_SC_PAGESIZE);

    if (log_init("../examples/log.cfg", "mmap_dev") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    debug("mapping_size: %lu", mapping_size);

    fd = open("/dev/calmwu_mmap_dev", O_RDWR);
    if (unlikely(fd < 0)) {
        fatal("open /dev/calmwu_mmap_dev failed, err: %s", strerror(errno));
    }

    mapped =
        mmap(NULL, mapping_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        fatal("mmap failed, err: %s", strerror(errno));
    } else {
        debug("mapped: %s", (char *)mapped);
    }

    // 解除映射
    if (mapped && munmap(mapped, mapping_size) != 0) {
        fatal("munmap failed, err: %s", strerror(errno));
    }

    close(fd);

    log_fini();
    return 0;
}
