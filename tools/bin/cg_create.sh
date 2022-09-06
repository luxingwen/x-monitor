#!/bin/bash

cgcreate -g memory:/hello-cg -g cpu:/hello-cg
# 限制内存使用为1G
cgset -r memory.limit_in_bytes=1024M /hello-cg
# 限制cpu的使用为1core
cgset -r cpu.cfs_period_us=100000 /hello-cg
cgset -r cpu.cfs_quota_us=100000 /hello-cg

cgget -g memory:/hello-cg 
cgget -g cpu:/hello-cg
