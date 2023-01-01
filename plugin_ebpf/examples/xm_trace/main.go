/*
 * @Author: CALM.WU
 * @Date: 2022-12-28 10:42:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-28 12:01:58
 */

package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	goflag "flag"
	"strconv"
	"time"

	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/cilium/ebpf/rlimit"
	"github.com/golang/glog"
	"xmonitor.calmwu/plugin_ebpf/examples/xm_trace/bpfmodule"

	calmutils "github.com/wubo0067/calmwu-go/utils"
)

const (
	_perf_max_stack_depth = 127
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

	calmutils.LoadKallSyms()

	ctx := calmutils.SetupSignalHandler()

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err)
	}

	spec, err := bpfmodule.LoadXMTrace()
	if err != nil {
		glog.Fatalf("failed to invoke LoadXMTrace, err: %v", err)
	}

	// rewrite .rodata, bpftool btf dump file xm_trace_syscalls.bpf.o
	err = spec.RewriteConstants(map[string]interface{}{
		"xm_trace_syscall_filter_pid": int32(_pid),
	})
	if err != nil {
		glog.Fatalf("failed to rewrite constants xm_trace_syscall_filter_pid, err: %v", err)
	}

	objs := bpfmodule.XMTraceObjects{}
	err = spec.LoadAndAssign(&objs, nil)
	if err != nil {
		glog.Fatalf("failed to load XMTraceObjects, err: %v", err)
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

	evt_rd, err := ringbuf.NewReader(objs.XmSyscallsEventMap)
	if err != nil {
		glog.Fatalf("failed to create ringbuf reader, error: %v", err)
	}
	defer evt_rd.Close()

	go func() {
		<-ctx.Done()

		if err := evt_rd.Close(); err != nil {
			glog.Fatalf("failed to close ringbuf reader, error: %v", err)
		}
	}()

	glog.Infof("Start receiving events...")

	var syscall_evt bpfmodule.XMTraceSyscallEvent
	for {
		record, err := evt_rd.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningln("Received signal, exiting...")
				return
			}
			glog.Errorf("reading from ringbuf failed, err: %v", err)
			continue
		}

		// parse the ringbuf record into XMTraceSyscallEvent struct
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, &syscall_evt); err != nil {
			glog.Errorf("failed to parse syscall event, err: %v", err)
			continue
		}

		// float64(syscall_evt.DelayNs)/float64(time.Millisecond)
		glog.Infof("pid:%d, tid:%d, (%f ms) syscall_nr:%d = %d",
			syscall_evt.Pid, syscall_evt.Tid, float64(syscall_evt.DelayNs)/float64(time.Millisecond),
			syscall_evt.SyscallNr, syscall_evt.SyscallRet)

		var ip [_perf_max_stack_depth]uint64

		err = objs.XmSyscallsStackMap.Lookup(syscall_evt.StackId, &ip)
		if err != nil {
			glog.Errorf("failed to lookup stack_id: %d, err: %v", syscall_evt.StackId, err)
		} else {
			for i := _perf_max_stack_depth - 1; i >= 0; i-- {
				if ip[i] == 0 {
					continue
				}
				addr := strconv.FormatUint(ip[i], 16)
				sym, err := calmutils.FindKsym(addr)
				if err != nil {
					sym = "?"
				}
				glog.Infof("\t0xip[%d]: %s\t%s", i, addr, sym)
			}
		}
	}

	glog.Infof("trace process syscalls exit")
}
