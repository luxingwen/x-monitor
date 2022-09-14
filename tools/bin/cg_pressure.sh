#!/bin/bash

# --vm 1个进程
# --vm-bytes 分配500m内存
# --vm-keep 一直占用内存
cgexec -g memory:/hello-cg -g cpu:/hello-cg stress-ng --vm 1 --vm-bytes 800000000 --vm-keep --timeout 10s --verbose