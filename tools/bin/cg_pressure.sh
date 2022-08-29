#!/bin/bash

# --vm 1个进程
# --vm-bytes 分配500m内存
# --vm-keep 一直占用内存
cgexec -g memory:/x-monitor -g cpu:/x-monitor stress --vm 1 --vm-bytes 500000000 --vm-keep --verbose