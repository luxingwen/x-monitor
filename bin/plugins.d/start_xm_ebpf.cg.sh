#!/bin/bash
# set -e
# set -u
# set -o pipefail

# 检查 cgroup 版本
check_cgroup_version() {
  if mount | grep -q "cgroup2"; then
    echo "cgroup V2"
  else
    echo "cgroup V1"
  fi
}

# 调用函数并将返回值存储在变量中
cgroup_version=$(check_cgroup_version)

if [[ "$cgroup_version" == "cgroup V1" ]]; then
    echo "system cgroup version is v1"
    cgcreate -g memory,cpu:/user.slice/x-monitor.ebpf

    # 限制内存使用为1G
    cgset -r memory.limit_in_bytes=2048M /user.slice/x-monitor.ebpf
    # 限制cpu的使用为1core
    cgset -r cpu.cfs_period_us=100000 /user.slice/x-monitor.ebpf
    cgset -r cpu.cfs_quota_us=100000 /user.slice/x-monitor.ebpf
    cgget -g memory,cpu:/user.slice/x-monitor.ebpf

    cgexec -g memory,cpu:/user.slice/x-monitor.ebpf nohup ./x-monitor.ebpf --config=../../env/config/xm_ebpf_plugin/config.yaml --pyroscope=http://127.0.0.1:4040/ --log_dir=/var/log/x-monitor/ --logtostderr=false --v=5 >/dev/null 2>&1 &
elif [[ "$cgroup_version" == "cgroup V2" ]]; then
    echo "system cgroup version is v2"
    cgcreate -g memory,cpu:/user.slice/x-monitor.ebpf

    # 限制cpu的使用为1core
    cgxset -2 -r cpu.max='100000 100000' /user.slice/x-monitor.ebpf
    # 限制memory上限，2G
    cgxset -2 -r memory.max=2147483648 /user.slice/x-monitor.ebpf
    # 不使用swap
    cgxset -2 -r memory.swap.max=0 /user.slice/x-monitor.ebpf
    cgxget -g memory,cpu:/user.slice/x-monitor.ebpf

    cgexec -g memory,cpu:/user.slice/x-monitor.ebpf nohup ./x-monitor.ebpf --config=../../env/config/xm_ebpf_plugin/config.yaml --pyroscope=http://127.0.0.1:4040/ --log_dir=/var/log/x-monitor/ --logtostderr=false --v=5 >/dev/null 2>&1 &
else
    echo "system cgroup version is unknown"
fi