#!/bin/bash

# 日志文件名
LOG_FILE="stap.txt"
# 锁文件名
LOCK_FILE="/tmp/stap_log.lock"

cleanup() {
    echo "Caught signal, terminating stap process..." >> $LOG_FILE
    kill -9 $STAP_PID
    rm -f $LOCK_FILE
    exit
}

# 检查是否已经有一个实例在运行
if [ -e $LOCK_FILE ]; then
    RUNNING_PID=$(cat $LOCK_FILE)
    if kill -0 $RUNNING_PID 2>/dev/null; then
        echo "Script is already running with PID $RUNNING_PID"
        exit 1
    else
        echo "Found stale lock file. Cleaning up."
        rm -f $LOCK_FILE
    fi
fi

# 创建锁文件并记录当前进程ID
echo $$ > $LOCK_FILE

# 设置信号处理
trap cleanup SIGINT SIGQUIT SIGTERM

# 运行 stap 命令并将输出重定向到日志文件
stap -v ./xfs_agf.stp 2>&1 | tee -a $LOG_FILE &

# 获取 stap 命令的进程ID
STAP_PID=$!

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

    # 删除旧的压缩文件，只保留最近8个
    ls -1tr stap_log_*.tar.gz | head -n -8 | xargs -r rm -f
done

# 等待 stap 进程结束
wait $STAP_PID

# 清理锁文件
rm -f $LOCK_FILE