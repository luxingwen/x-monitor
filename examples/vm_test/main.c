/*
 * @Author: CALM.WU
 * @Date: 2023-04-07 10:56:11
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-07 17:37:42
 */

// bpftrace do_mmap的测试目标

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/strings.h"

#define BUF_SIZE 4095
#define MMAP_SIZE(x) ((1 << x) << 12)

static const struct option __opts[] = {
    { "mmap_pri_anon", no_argument, NULL, 'n' },
    { "mmap_file", no_argument, NULL, 'f' },
    { "brk", no_argument, NULL, 'b' },
    { "verbose", no_argument, NULL, 'v' },
};

static void mmap_anonymous_test() {
    // allocate memory using anonymous mmap
    size_t mmap_size = MMAP_SIZE(2);

    sleep(15);

    debug("start mmap_anonymous_test...");
    char *mapped = (char *)mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // write some data to the mapped memory
    strlcpy(mapped, "Hello, world!", mmap_size);
    debug("mmap_anonymous_test mmap %lu bytes and write data to mapped region: "
          "%p",
          mmap_size, mapped);
    sleep(15);

    // unmap the memory
    if (munmap(mapped, mmap_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
    debug("mmap_anonymous_test munmap anonymous buf:%p", mapped);
    sleep(15);
    debug("stop mmap_anonymous_test...");
}

static void mmap_file_test() {
    debug("start mmap_file_test...");

    sleep(10);

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
    debug("mmap_file_test open and truncate file: vmalloc_process.txt");
    sleep(5);

    // map the file into memory
    char *mapped = (char *)mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // write some data to the mapped region
    strlcpy(mapped, "Hello, world!", mmap_size);
    debug("mmap_file_test mmap %lu size and write data to mapped region: %p",
          mmap_size, mapped);
    sleep(10);

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
    debug("mmap_file_test munmap mapped region: %p", mapped);
    sleep(10);

    // close the file
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
    debug("stop mmap_file_test...");
}

void brk_test() {
    debug("start brk_test...");

    sleep(10);
    // Address Perm   Offset Device    Inode  Size  Rss Pss Referenced Anonymous
    // LazyFree ShmemPmdMapped FilePmdMapped Shared_Hugetlb Private_Hugetlb Swap
    // SwapPss Locked THPeligible Mapping
    // 018df000 rw-p 00000000  00:00        0 264  136 136        136 136 0 0 0
    // 0               0    0 0 0           0 [heap]

    char *alloc_bufs[10] = { 0 };
    for (int32_t i = 0; i < 10; i++) {
        alloc_bufs[i] = (char *)malloc(BUF_SIZE);
        memset(alloc_bufs[i], 0, BUF_SIZE);
    }

    debug("brk_test: first buf address:%p", alloc_bufs[0]);
    // first buf address:0x1900250

    debug("brk_test: malloc 10 * %d bytes", BUF_SIZE);
    // 018df000 rw-p 00000000  00:00        0   264  176 176        176 176 0 0
    // 0              0               0    0       0      0           0 [heap]

    sleep(30);

    for (int32_t i = 0; i < 10; i++) {
        free(alloc_bufs[i]);
    }

    debug("brk_test: free 10 * %d bytes", BUF_SIZE);
    // 018df000 rw-p 00000000  00:00        0   264  176 176        176 176 0 0
    // 0              0               0    0       0      0           0 [heap]

    sleep(30);

    int32_t ret = malloc_trim(10 * BUF_SIZE);

    debug("brk_test: malloc_trim(10 * %d bytes) = %d", BUF_SIZE, ret);
    // 018df000 rw-p 00000000  00:00        0   176  176 176        176 176 0 0
    // 0              0               0    0       0      0           0 [heap]

    sleep(10);

    debug("stop brk_test...");
}

static void __usage() {
    debug("\nvmm_test [options]\n"
          "OPTIONS:\n"
          "\t -n\t use mmap private annoyous\n"
          "\t -f\t use mmap file\n"
          "\t -b\t use brk\n");
}

int32_t main(int32_t argc, char **argv) {
    if (log_init("../examples/log.cfg", "alloc_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    debug("pid:%d", getpid());

    int32_t option;
    while (-1 != (option = getopt_long(argc, argv, "nfb", __opts, NULL))) {
        switch (option) {
        case 'n':
            mmap_anonymous_test();
            goto finish;
        case 'f':
            mmap_file_test();
            goto finish;
        case 'b':
            brk_test();
            goto finish;
        case 'v':
        default:
            break;
        }
    }

    if (-1 == option) {
        __usage();
    }
finish:
    log_fini();
    return 0;
}