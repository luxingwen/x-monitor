/*
 * @Author: CALM.WU
 * @Date: 2022-12-28 10:42:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-28 12:01:58
 */

package main

import (
	goflag "flag"

	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/rlimit"
	"github.com/golang/glog"
	"xmonitor.calmwu/plugin_ebpf/examples/trace_process_syscalls/bpfmodule"

	calmutils "github.com/wubo0067/calmwu-go/utils"
)

var (
	_pid int
)

func init() {
	goflag.IntVar(&_pid, "pid", 0, "trace process syscalls pid")
}

func main() {
	goflag.Parse()

	glog.Infof("start trace process:'%d' syscalls", _pid)

	ctx := calmutils.SetupSignalHandler()

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err)
	}

	spec, err := bpfmodule.LoadTraceProcessSyscalls()
	if err != nil {
		glog.Fatalf("failed to invoke LoadTraceProcessSyscalls, err: %v", err)
	}

	// rewrite .rodata, bpftool btf dump file xm_trace_syscalls.bpf.o
	err = spec.RewriteConstants(map[string]interface{}{
		"xm_trace_syscall_filter_pid": int32(_pid),
	})
	if err != nil {
		glog.Fatalf("failed to rewrite constants xm_trace_syscall_filter_pid, err: %v", err)
	}

	objs := bpfmodule.TraceProcessSyscallsObjects{}
	err = spec.LoadAndAssign(&objs, nil)
	if err != nil {
		glog.Fatalf("failed to load trace_process_syscalls: %v", err)
	}

	defer objs.Close()

	// XmBtfRtpSysEnter name:Tracing(xm_btf_rtp__sys_enter)#12 type:26
	glog.Infof("XmBtfRtpSysEnter name:%s type:%d", objs.XmBtfRtpSysEnter.String(), objs.XmBtfRtpSysEnter.Type())

	// attach btf raw tracepoint program for link
	link_sys_enter, err := link.AttachTracing(link.TracingOptions{Program: objs.XmBtfRtpSysEnter})
	if err != nil {
		glog.Fatalf("failed to attach %s program for link, error: %v", objs.XmBtfRtpSysEnter.String(), err)
	}
	defer link_sys_enter.Close()

	link_sys_exit, err := link.AttachTracing(link.TracingOptions{Program: objs.XmBtfRtpSysExit})
	if err != nil {
		glog.Fatalf("failed to attach %s program for link, error: %v", objs.XmBtfRtpSysExit.String(), err)
	}
	defer link_sys_exit.Close()

	<-ctx.Done()

	glog.Infof("trace process syscalls exit")
}
