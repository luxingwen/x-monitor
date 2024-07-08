/*
 * @Author: CALM.WU
 * @Date: 2024-07-08 14:54:56
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-07-08 17:02:34
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/strings.h"

#include "../dev_ioctl_cmd.h"

static const struct option __opts[] = {
    { "dev", required_argument, NULL, 'd' },
    { "cmd", required_argument, NULL, 'c' },
    { "len", required_argument, NULL, 'l' },
    { "verbose", no_argument, NULL, 'v' },
    { 0, 0, 0, 0 },
};

static const char *__optstr = "d:c:l:v";

struct args {
    const char *dev;
    size_t request;
    int32_t len;
    bool verbose;
} __env = {
    .request = CW_IOCTL_RESET,
    .len = 0,
};

static void usage(const char *prog)
{
    fprintf(stderr,
            "usage: %s [OPTS]\n"
            "OPTS:\n"
            "    -d    char device(/dev/xxx)\n"
            "    -c    reset/read/write\n"
            "    -l    write bytes\n"
            "    -v    verbose\n",
            prog);
}

int32_t main(int32_t argc, char *argv[])
{
    int32_t ret = EXIT_SUCCESS, fd;
    int32_t opt = 0;

    while ((opt = getopt_long(argc, argv, __optstr, __opts, NULL)) != -1) {
        switch (opt) {
        case 'v':
            __env.verbose = true;
            break;
        case 'd':
            __env.dev = strndup(optarg, strlen(optarg));
            break;
        case 'c':
            if (strncmp("reset", optarg, strlen(optarg)) == 0) {
                __env.request = CW_IOCTL_RESET;
            } else if (strncmp("read", optarg, strlen(optarg)) == 0) {
                __env.request = CW_IOCTL_GET_LENS;
            } else if (strncmp("write", optarg, strlen(optarg)) == 0) {
                __env.request = CW_IOCTL_SET_LENS;
            } else {
                fprintf(stderr, "Unrecognized command '%s'\n", optarg);
                usage(basename(argv[0]));
                exit(EXIT_FAILURE);
            }
            break;
        case 'l':
            __env.len = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Unrecognized option '%c'\n", opt);
            usage(basename(argv[0]));
            exit(EXIT_FAILURE);
        }
    }

    if (unlikely(NULL == __env.dev)) {
        fprintf(stderr, "device is required\n");
        usage(basename(argv[0]));
        exit(EXIT_FAILURE);
    }

    if (log_init("../examples/log.cfg", "ioctl_test") != 0) {
        fprintf(stderr, "log init failed\n");
        exit(EXIT_FAILURE);
    }

    // 打开设备
    if ((fd = open(__env.dev, O_RDWR, 0)) == -1) {
        error("open %s failed. err:%s", __env.dev, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    // 执行 ioctl
    switch (__env.request) {
    case CW_IOCTL_RESET:
        if (ioctl(fd, CW_IOCTL_RESET, 0) == -1) {
            error("ioctl reset '%s' failed. err:%s", __env.dev,
                  strerror(errno));
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        info("ioctl reset '%s' success.", __env.dev);
        break;
    case CW_IOCTL_GET_LENS:
        if (ioctl(fd, CW_IOCTL_GET_LENS, &__env.len) == -1) {
            error("ioctl get '%s' failed. err:%s", __env.dev, strerror(errno));
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        info("ioctl get '%s' success. len:%d", __env.dev, __env.len);
        break;
    case CW_IOCTL_SET_LENS:
        if (ioctl(fd, CW_IOCTL_SET_LENS, &__env.len) == -1) {
            error("ioctl set '%s' failed. err:%s", __env.dev, strerror(errno));
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        info("ioctl set '%s' success. len:%d", __env.dev, __env.len);
        break;
    default:
        error("Unrecognized command '%ld'", __env.request);
        ret = EXIT_FAILURE;
        goto cleanup;
    }

cleanup:
    free((void *)__env.dev);

    if (-1 != fd) {
        close(fd);
    }
    log_fini();
    exit(ret);
}
