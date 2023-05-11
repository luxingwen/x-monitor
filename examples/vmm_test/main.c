/*
 * @Author: CALM.WU
 * @Date: 2023-04-07 10:56:11
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-07 17:37:42
 */

// bpftrace do_mmap的测试目标

#include "utils/common.h"
#include "utils/compiler.h"
#include <malloc.h>
#include "utils/log.h"
#include "utils/strings.h"

#define BUF_SIZE 4095
#define MMAP_SIZE(x) ((1 << x) << 12)

static char *alloc_list[4] = { 0 };

static void calloc_list() {
    for (int32_t i = 0; i < 4; i++) {
        alloc_list[i] = (char *)calloc(BUF_SIZE, 1);
        strlcpy(alloc_list[i], "Hello, world!", BUF_SIZE);
    }
}

static void free_list() {
    for (int32_t i = 0; i < 4; i++) {
        free(alloc_list[i]);
    }
    malloc_trim(0);
}

static void mmap_anonymous_test() {
    // allocate memory using anonymous mmap
    size_t mmap_size = MMAP_SIZE(2);

    char *buf = (char *)mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // write some data to the mapped memory
    strlcpy(buf, "Hello, world!", mmap_size);

    debug("mmap anonymous buf: %p, %lu bytes", buf, mmap_size);
    sleep(10);

    // unmap the memory
    if (munmap(buf, mmap_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
    debug("munmap anonymous buf: %p", buf);
}

static void mmap_file_test() {
    size_t mmap_size = MMAP_SIZE(2);

    int32_t fd =
        open("vmalloc_process.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, BUF_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // map the file into memory
    char *mapped = (char *)mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // write some data to the mapped region
    strlcpy(mapped, "Hello, world!", mmap_size);

    // flush the changes to the file
    if (msync(mapped, mmap_size, MS_SYNC) == -1) {
        perror("msync");
        exit(EXIT_FAILURE);
    }

    // unmap the memory region
    if (munmap(mapped, mmap_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    // close the file
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}

int32_t main(int32_t argc, char **argv) {
    if (log_init("../examples/log.cfg", "alloc_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    debug("pid:%d", getpid());

    sleep(10);
    malloc_trim(0);

    debug("start alloc...");

    for (int i = 0; i < 5; i++) {
        calloc_list();
        sleep(1);
        mmap_anonymous_test();
        // sleep(1);
        // mmap_file_test();
        // sleep(1);
        free_list();
    }

    debug("stop alloc...");
    log_fini();
    return 0;
}