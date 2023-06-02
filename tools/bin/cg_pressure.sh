#!/bin/bash

# --vm 1个进程
# --vm-bytes 分配500m内存
# --vm-keep 一直占用内存
cgexec -g cpu,memory:/user.slice/hello-cg stress-ng --vm 1 --vm-bytes 100M --vm-hang 0 --vm-keep --timeout 100s --iterations 11 --verbose
