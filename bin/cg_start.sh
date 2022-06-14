#!/bin/bash
# set -e
# set -u 
# set -o pipefail

cgcreate -g memory:/x-monitor -g cpu:/x-monitor
# 限制内存使用为1G
cgset -r memory.limit_in_bytes=1024M /x-monitor
# 限制cpu的使用为1core
cgset -r cpu.cfs_period_us=100000 /x-monitor
cgset -r cpu.cfs_quota_us=100000 /x-monitor

cgget -g memory:/x-monitor 
cgget -g cpu:/x-monitor

cgexec -g memory:/x-monitor -g cpu:/x-monitor ./x-monitor -c ../env/config/x-monitor.cfg

