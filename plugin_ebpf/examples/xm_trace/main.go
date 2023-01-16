/*
 * @Author: CALM.WU
 * @Date: 2022-12-28 10:42:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-01-09 15:27:17
 */

package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	goflag "flag"
	"reflect"
	"strings"
	"time"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/cilium/ebpf/rlimit"
	"github.com/golang/glog"
	"xmonitor.calmwu/plugin_ebpf/examples/xm_trace/bpfmodule"

	calmutils "github.com/wubo0067/calmwu-go/utils"
)

const (
	_perf_max_stack_depth = 20
)

var _pid int

func init() {
	goflag.IntVar(&_pid, "pid", 0, "trace process syscalls pid")
}

func main() {
	goflag.Parse()

	glog.Infof("start trace process:'%d' syscalls", _pid)

	calmutils.LoadKallSyms()
	procSyms, _ := calmutils.NewProcSyms(_pid)

	ctx := calmutils.SetupSignalHandler()

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err)
	}

	// 加载字节码，生成ebpf.ProgramSpec
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

	glog.Infof("XmBtfRtpSysEnter name:%s type:%d", objs.XmTraceRawTpSysEnter.String(), objs.XmTraceRawTpSysEnter.Type())

	defer objs.Close()

	// 通过reflect来获取Programs的所有成员，统一ebpf Program的attach
	var ebpfLinks []link.Link

	bpfProgs := reflect.Indirect(reflect.ValueOf(objs.XMTracePrograms))
	bpfProgsT := bpfProgs.Type()

	numFields := bpfProgsT.NumField()
	for i := 0; i < numFields; i++ {
		bpfProgS := bpfProgsT.Field(i)                  // reflect.StructField
		bpfProgV := bpfProgs.FieldByName(bpfProgS.Name) // 成员具体值

		// glog.Infof("epbf Prog name[%s] type:[%s] value:[%v] valueType:[%v] kind:%v\n",
		// 	bpfProgS.Name, bpfProgS.Type.String(), bpfProgV, bpfProgV.Type(), bpfProgS.Type.Kind())

		if bpfProgS.Type.Kind() == reflect.Ptr {
			bpfProg := bpfProgV.Interface().(*ebpf.Program)

			progStr := bpfProg.String()
			startPos := strings.Index(progStr, "__")
			endPos := strings.LastIndexByte(progStr, ')')
			progSecSubName := progStr[startPos+2 : endPos]

			glog.Infof("prog:'%s' ebpf info:'%s'", bpfProgS.Name, bpfProg.String())

			switch bpfProg.Type() {
			case ebpf.Kprobe:
				// attach kprobe program
				linkKP, err := link.Kprobe(progSecSubName, bpfProg, nil)
				if err != nil {
					glog.Fatalf("failed to attach %s program for link, error: %v", bpfProg.String(), err)
				} else {
					glog.Infof("attach KProbe %s program for link successed.", bpfProg.String())
					ebpfLinks = append(ebpfLinks, linkKP)
				}
			case ebpf.RawTracepoint:
				linkRawTP, err := link.AttachRawTracepoint(link.RawTracepointOptions{Name: progSecSubName, Program: bpfProg})
				if err != nil {
					glog.Fatalf("failed to attach %s program for link, error: %v", bpfProg.String(), err)
				} else {
					glog.Infof("attach RawTracepoint %s program for link successed.", bpfProg.String())
					ebpfLinks = append(ebpfLinks, linkRawTP)
				}
			case ebpf.Tracing:
				linkTracing, err := link.AttachTracing(link.TracingOptions{Program: bpfProg})
				if err != nil {
					glog.Fatalf("failed to attach %s program for link, error: %v", bpfProg.String(), err)
				} else {
					glog.Infof("attach BTFRawTracepoint %s program for link successed.", bpfProg.String())
					ebpfLinks = append(ebpfLinks, linkTracing)
				}
			default:
				glog.Fatalf("unsupported ebpf type: %d", bpfProg.Type())
			}
		}
	}

	defer func() {
		for i, link := range ebpfLinks {
			glog.Infof("%d link detached", i)
			link.Close()
		}
	}()

	// attach btf raw tracepoint program for link
	// link_sys_enter, err := link.AttachTracing(link.TracingOptions{Program: objs.XmTraceRtpSysEnter})

	// attach raw_tracepoint program
	// link_sys_enter, err := link.AttachRawTracepoint(link.RawTracepointOptions{Name: "sys_enter", Program: objs.XmTraceRawTpSysEnter})
	// if err != nil {
	// 	glog.Fatalf("failed to attach %s program for link, error: %v", objs.XmTraceRawTpSysEnter.String(), err)
	// }
	// defer link_sys_enter.Close()

	// // link_sys_exit, err := link.AttachTracing(link.TracingOptions{Program: objs.XmTraceRtpSysExit})
	// link_sys_exit, err := link.AttachRawTracepoint(link.RawTracepointOptions{Name: "sys_exit", Program: objs.XmTraceRawTpSysExit})
	// if err != nil {
	// 	glog.Fatalf("failed to attach %s program for link, error: %v", objs.XmTraceRawTpSysExit.String(), err)
	// }
	// defer link_sys_exit.Close()

	// link_kp_sys_readlinkat, err := link.Kprobe("sys_readlinkat", objs.XmTraceKpSysReadlinkat, nil)
	// if err != nil {
	// 	glog.Fatalf("failed to attach kprobe %s program for link, error: %v", objs.XmTraceKpSysReadlinkat.String(), err)
	// }
	// defer link_kp_sys_readlinkat.Close()

	// link_kp_sys_openat, err := link.Kprobe("sys_openat", objs.XmTraceKpSysOpenat, nil)
	// if err != nil {
	// 	glog.Fatalf("failed to attach kprobe %s program for link, error: %v", objs.XmTraceKpSysOpenat.String(), err)
	// }
	// defer link_kp_sys_openat.Close()

	// link_kp_sys_close, err := link.Kprobe("sys_close", objs.XmTraceKpSysClose, nil)
	// if err != nil {
	// 	glog.Fatalf("failed to attach kprobe %s program for link, error: %v", objs.XmTraceKpSysClose.String(), err)
	// }
	// defer link_kp_sys_close.Close()

	evtRD, err := ringbuf.NewReader(objs.XmSyscallsEventMap)
	if err != nil {
		glog.Fatalf("failed to create ringbuf reader, error: %v", err)
	}
	defer evtRD.Close()

	go func() {
		<-ctx.Done()

		if err := evtRD.Close(); err != nil {
			glog.Fatalf("failed to close ringbuf reader, error: %v", err)
		}
	}()

	glog.Infof("Start receiving events...")
	// stackFrameSize := (strconv.IntSize / 8)

	var syscallEvt bpfmodule.XMTraceSyscallEvent
	for {
		record, err := evtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningln("Received signal, exiting...")
				return
			}
			glog.Errorf("reading from ringbuf failed, err: %v", err)
			continue
		}

		// parse the ringbuf record into XMTraceSyscallEvent struct
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, &syscallEvt); err != nil {
			glog.Errorf("failed to parse syscall event, err: %v", err)
			continue
		}

		// float64(syscallEvt.DelayNs)/float64(time.Millisecond)
		glog.Infof("pid:%d, tid:%d, (%f ms) syscall_nr:%d = %d",
			syscallEvt.Pid, syscallEvt.Tid, float64(syscallEvt.CallDelayNs)/float64(time.Millisecond),
			syscallEvt.SyscallNr, syscallEvt.SyscallRet)

		var stackAddrs [_perf_max_stack_depth]uint64

		if syscallEvt.KernelStackId > 0 {
			// stackBytes, err := objs.XmSyscallsStackMap.LookupBytes(syscallEvt.StackId)
			err = objs.XmSyscallsStackMap.Lookup(syscallEvt.KernelStackId, &stackAddrs)
			if err != nil {
				glog.Errorf("failed to lookup stack_id: %d, err: %v", syscallEvt.KernelStackId, err)
			} else {
				// for i := 0; i < _perf_max_stack_depth; i++ {
				for i, stackAddr := range stackAddrs {
					// for i := 0; i < len(stackBytes); i += stackFrameSize {
					// stackAddr := binary.LittleEndian.Uint64(stackBytes[i : i+stackFrameSize])
					if stackAddr == 0 {
						continue
					}
					sym, offset, err := calmutils.FindKsym(stackAddr)
					if err != nil {
						sym = "?"
					}
					glog.Infof("\tip[%d]: 0x%016x\t%s+0x%x", i, stackAddr, sym, offset)
				}
			}
		}

		if syscallEvt.UserStackId > 0 {
			err = objs.XmSyscallsStackMap.Lookup(syscallEvt.UserStackId, &stackAddrs)
			if err != nil {
				glog.Errorf("failed to lookup stack_id: %d, err: %v", syscallEvt.UserStackId, err)
			} else {
				// for i := 0; i < _perf_max_stack_depth; i++ {
				for i, stackAddr := range stackAddrs {
					// for i := 0; i < len(stackBytes); i += stackFrameSize {
					// stackAddr := binary.LittleEndian.Uint64(stackBytes[i : i+stackFrameSize])
					if stackAddr == 0 {
						continue
					}
					sym, offset, moduleName, err := procSyms.FindPsym(stackAddr)
					if err != nil {
						sym = "?"
					}
					glog.Infof("\tip[%d]: 0x%016x\t%s+0x%x [%s]", i, stackAddr, sym, offset, moduleName)
				}
			}
		}

		// objs.XmSyscallsStackMap.Delete(syscallEvt.StackId)
	}

	glog.Infof("trace process syscalls exit")
}
