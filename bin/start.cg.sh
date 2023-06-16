#!/bin/bash
# set -e
# set -u 
# set -o pipefail

cat /proc/cmdline | grep -q "systemd.unified_cgroup_hierarchy=1"
if [ $? -ne 0 ] ;then
    echo "system cgroup version is v1"

    cgcreate -g memory,cpu:/system.slice/x-monitor
    # 限制内存使用为1G
    cgset -r memory.limit_in_bytes=1024M /system.slice/x-monitor
    # 限制cpu的使用为1core
    cgset -r cpu.cfs_period_us=100000 /system.slice/x-monitor
    cgset -r cpu.cfs_quota_us=100000 /system.slice/x-monitor

    cgget -g memory,cpu:/system.slice/x-monitor 

    cgexec -g memory,cpu:/system.slice/x-monitor ./x-monitor -c ../env/config/x-monitor.cfg
else
    echo "system cgroup version is v2"

    cgcreate -g memory,cpu:/x-monitor
    # 限制cpu的使用为1core
    cgxset -2 -r cpu.max='100000 100000' x-monitor 
    # 限制memory上限，1G
    cgxset -2 -r memory.max=1073741824 x-monitor
    # 不使用swap
    cgxset -2 -r memory.swap.max=0 x-monitor
    
    cgxget -g memory,cpu:/x-monitor

    cgexec -g memory,cpu:/x-monitor ./x-monitor -c ../env/config/x-monitor.cfg
fi

