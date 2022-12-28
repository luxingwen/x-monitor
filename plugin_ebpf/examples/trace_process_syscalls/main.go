/*
 * @Author: CALM.WU
 * @Date: 2022-12-28 10:42:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-28 12:01:58
 */

package main

import (
	goflag "flag"

	"github.com/cilium/ebpf/rlimit"
	"github.com/golang/glog"
	"xmonitor.calmwu/plugin_ebpf/examples/trace_process_syscalls/bpfmodule"
)

func main() {
	goflag.Parse()

	glog.Infof("trace process syscalls")

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err)
	}

	objs := bpfmodule.TraceProcessSyscallsObjects{}
	if err := bpfmodule.LoadTraceProcessSyscallsObjects(&objs, nil); err != nil {
		glog.Fatalf("failed to load trace_process_syscalls: %v", err)
	}

	defer objs.Close()

	glog.Infof("trace process syscalls exit")
}
