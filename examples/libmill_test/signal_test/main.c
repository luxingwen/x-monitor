/*
 * @Author: calmwu
 * @Date: 2022-09-04 15:16:26
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-09-04 16:30:44
 */

#include "utils/common.h"
#include "utils/log.h"
#include "libmill/libmill.h"

// 全双工的，不分每个fd都可以读写
static int32_t __signal_sock[2] = { 0, 0 };

static void __signal_handler(int32_t signal_no) {
    char    b = (char)signal_no;
    int32_t events = fdwait(__signal_sock[0], FDW_OUT, -1);
    if (events & FDW_OUT) {
        ssize_t sz = write(__signal_sock[0], &b, 1);
        debug("signal_hander write signal %d to signal_sock[0] sz: %ld", signal_no, sz);
    }
    return;
}

coroutine void sender(chan ch) {
    // 读取信号
    char  signo = chr(ch, char);
    pid_t pid = getpid();
    kill(pid, signo);
    debug("read signo:%d from chan then use kill trigger pid: %d signal_handler", signo, pid);
    chclose(ch);
}

coroutine void recevier(chan ch) {   // 读取socket
    int32_t events = fdwait(__signal_sock[1], FDW_IN, -1);
    if (events & FDW_IN) {
        char    signo = 0;
        ssize_t sz = read(__signal_sock[1], &signo, 1);
        debug("read signal %d from signal_sock[1] sz: %ld", signo, sz);
        chs(ch, char, signo);
        debug("send signal %d to chan", signo);
    }
    chclose(ch);
}

int32_t main(int32_t argc, char *argv[]) {
    if (log_init("../examples/log.cfg", "libmill_signal_test") != 0) {
        fprintf(stderr, "log init failed");
        return -1;
    }

    goprepare(10, 25000, 300);

    char *buff = calloc(1024, 1);

    int32_t ret = 0;
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, __signal_sock);
    if (ret < 0) {
        error("socketpair failed");
        return -1;
    }

    // set nonblock
    int32_t opt = fcntl(__signal_sock[0], F_GETFL);
    fcntl(__signal_sock[0], F_SETFL, opt | O_NONBLOCK);
    fcntl(__signal_sock[1], F_SETFL, opt | O_NONBLOCK);

    // 设置信号处理函数
    signal(SIGUSR1, __signal_handler);

    // 创建chan
    chan send_sig_ch = chmake(char, 0);
    chan recv_sig_ch = chmake(char, 0);

    for (int32_t i = 0; i < 1; i++) {
        // 创建协程
        go(sender(chdup(send_sig_ch)));
        go(recevier(chdup(recv_sig_ch)));

        debug("---main process send signal SIGUSR1");

        chs(send_sig_ch, char, SIGUSR1);
        int32_t signo = chr(recv_sig_ch, char);

        debug("<---main process recv signal %s", (signo == 10) ? "SIGUSR1" : "unknown");
    }

    chclose(send_sig_ch);
    chclose(recv_sig_ch);

    debug("main process sleep...");

    sleep(10);

    return 0;
}