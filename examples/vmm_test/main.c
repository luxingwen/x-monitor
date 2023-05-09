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

#define BUF_SIZE 1024
#define MMAP_SIZE 256

static void malloc_test() {
    char *buf[4];

    for (int32_t i = 0; i < 4; i++) {
        buf[i] = (char *)calloc(BUF_SIZE, 1);
        strlcpy(buf[i], "Hello, world!", BUF_SIZE);
    }

    for (int32_t i = 0; i < 4; i++) {
        free(buf[i]);
    }
}

static void mmap_anonymous_test() {
    // allocate memory using anonymous mmap
    char *buf = (char *)mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // write some data to the mapped memory
    strlcpy(buf, "Hello, world!", MMAP_SIZE);

    // unmap the memory
    if (munmap(buf, MMAP_SIZE) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
}

static void mmap_file_test() {
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
    char *mapped = (char *)mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // write some data to the mapped region
    strlcpy(mapped, "Hello, world!", MMAP_SIZE);

    // flush the changes to the file
    if (msync(mapped, MMAP_SIZE, MS_SYNC) == -1) {
        perror("msync");
        exit(EXIT_FAILURE);
    }

    // unmap the memory region
    if (munmap(mapped, MMAP_SIZE) == -1) {
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
    malloc_trim(0);

    sleep(10);

    debug("start alloc...");

    for (int i = 0; i < 5; i++) {
        malloc_test();
        sleep(1);
        mmap_anonymous_test();
        sleep(1);
        mmap_file_test();
        sleep(1);
    }

    debug("stop alloc...");
    log_fini();
    return 0;
}