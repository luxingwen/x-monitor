#!/bin/bash

# 指定目录路径
dir_path="/sys/kernel/btf"

# 循环遍历目录下的所有文件
for file in $dir_path/*
do
    # 检查文件是否是普通文件
    if [ -f "$file" ]
    then
        # 提取文件名并作为参数执行bpftool命令
        filename=$(basename "$file")
        bpftool btf dump file "$file" format c
    fi
done