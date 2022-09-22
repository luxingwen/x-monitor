#!/bin/bash

# --vm 1个进程
# --vm-bytes 分配500m内存
# --vm-keep 一直占用内存
cgexec -g memory,cpu:/hello-cg stress-ng --vm 1 --vm-bytes 949450000 --vm-keep --timeout 100s --verbose
