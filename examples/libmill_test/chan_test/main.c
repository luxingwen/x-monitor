/*
 * @Author: calmwu
 * @Date: 2022-09-03 16:39:45
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-09-03 17:37:46
 */

#include "utils/common.h"
#include "utils/log.h"
#include "libmill/libmill.h"

coroutine void receiver(chan ch_num, chan ch_exit) {
    int32_t exit_flag = 0;

    while (1) {
        choose {
            in(ch_num, int32_t *, p) : debug("receive int32_t* p=%d", *p);
            in(ch_exit, int32_t, b_exit) : {
                debug("receive exit, b_exit: %d", b_exit);
                exit_flag = 1;
                break;
            }
            end
        }
        if (exit_flag) {
            break;
        }
    }

    chclose(ch_num);
    chclose(ch_exit);

    debug("receiver coroutine exit");
}

int32_t main(int32_t argc, char *argv[]) {
    if (log_init("../examples/log.cfg", "libmill_chan_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    // 创建一个chan
    chan ch_num = chmake(int32_t *, 0);
    chan ch_exit = chmake(int32_t, 0);

    go(receiver(chdup(ch_num), chdup(ch_exit)));

    int32_t *p_num = (int32_t *)calloc(1, sizeof(int32_t));
    *p_num = 100;

    for (int32_t i = 0; i < 10; i++) {
        chs(ch_num, int32_t *, p_num);
        msleep(now() + 100);
        --(*p_num);
    }
    // receiver 会在该ch_exit上一直读取到数据。
    chdone(ch_exit, int, 999);
    debug("chdone to ch_exit");

    chclose(ch_num);
    chclose(ch_exit);

    msleep(now() + 1000);

    debug("main coroutine exit");

    free(p_num);

    log_fini();
    return 0;
}

// http://www.ictbanking.com/LJ/268/12002.html