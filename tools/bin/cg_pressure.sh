#!/bin/bash

# --vm 1个进程
# --vm-bytes 分配500m内存
# --vm-keep 一直占用内存
while true; do
    cgexec -g cpu,memory:/user.slice/hello-cg stress-ng --vm 1 --vm-bytes 1.5G --vm-hang 0 --vm-keep --timeout 3s --verbose
    sleep 2
done
