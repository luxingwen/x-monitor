/*
 * @Author: calmwu
 * @Date: 2022-07-14 22:38:50
 * @Last Modified by:   calmwu
 * @Last Modified time: 2022-07-14 22:38:50
 */

package main

import (
	"go.uber.org/automaxprocs/maxprocs"
	"xmonitor.calmwu/plugin_ebpf/collector/cmd"
)

// Main is the entry point for the command
func main() {
	undo, _ := maxprocs.Set()
	defer undo()
	cmd.Main()
}

// ./xm-monitor.eBPF --log_dir="/var/log/x-monitor/" --v=3
