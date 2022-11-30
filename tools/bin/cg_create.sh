#!/bin/bash

cat /proc/cmdline | grep -q "systemd.unified_cgroup_hierarchy=1"
if [ $? -ne 0 ] ;then
    echo "system cgroup version is v1"
    cgcreate -g memory:/user.slice/hello-cg -g cpu:/user.slice/hello-cg
    # 限制内存使用为1G
    cgset -r memory.limit_in_bytes=1024M /user.slice/hello-cg
    # 限制cpu的使用为1core
    cgset -r cpu.cfs_period_us=100000 /user.slice/hello-cg
    cgset -r cpu.cfs_quota_us=100000 /user.slice/hello-cg

    cgget -g memory:/user.slice/hello-cg 
    cgget -g cpu:/user.slice/hello-cg
else
    echo "system cgroup version is v2"

    cgcreate -g memory,cpu:/user.slice/hello-cg
    # 限制cpu的使用为1core
    cgxset -2 -r cpu.max='50000 100000' hello-cg 
    # 限制memory上限，1G
    cgxset -2 -r memory.max=1073741824 hello-cg
    # 不使用swap
    cgxset -2 -r memory.swap.max=0 hello-cg
    
    cgxget -g memory,cpu:/user.slice/hello-cg    
fi


