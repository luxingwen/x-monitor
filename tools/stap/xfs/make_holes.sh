#!/bin/bash

# 创建一个 400MB 的文件
fallocate -l 400M largefile

# 在文件中插入 40 个 5MB 的洞
for i in {0..39}
do
    start_offset=$((i * 10 + 1))M  # 每个洞之间相隔 5MB
    fallocate -p -o "${start_offset}" -l 5M largefile
done
