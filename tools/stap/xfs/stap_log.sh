#!/bin/bash

# 日志文件名
LOG_FILE="stap.txt"

# 运行 stap 命令并将输出重定向到日志文件
stap -v ./xfs_agf.stp 2>&1 | tee -a $LOG_FILE &

# 获取 stap 命令的进程ID
STAP_PID=$!

# 设置信号处理
trap "echo \"Caught signal, terminating stap process...\" >> $LOG_FILE; kill -9 $STAP_PID; exit" SIGINT SIGQUIT SIGTERM

# 每3小时压缩一次日志文件
while true; do
    for ((i=0; i<10800; i++)); do
        sleep 1
        kill -0 $STAP_PID 2>/dev/null || exit
    done

    # 获取当前时间
    TIMESTAMP=$(date +"%Y%m%d%H%M")
    # 压缩日志文件
    tar -czf "stap_log_$TIMESTAMP.tar.gz" $LOG_FILE
    # 清空原始日志文件
    > $LOG_FILE
done

# 等待 stap 进程结束
wait $STAP_PID