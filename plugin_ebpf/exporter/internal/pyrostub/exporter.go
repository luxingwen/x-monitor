/*
 * @Author: CALM.WU
 * @Date: 2023-09-04 15:21:06
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-12-13 15:25:00
 */

package pyrostub

import (
	"bytes"
	"context"
	"encoding/binary"
	"fmt"
	"os"
	"sync"

	"github.com/go-kit/log"
	"github.com/golang/glog"
	"github.com/grafana/agent/component"
	"github.com/grafana/agent/component/pyroscope"
	"github.com/grafana/agent/component/pyroscope/write"
	"github.com/grafana/pyroscope/ebpf/pprof"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/samber/lo"
	"github.com/sanity-io/litter"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/frame"
)

const (
	__defaultAgentIDFmt    = "x-monitor.ebpf.%s"
	__perf_max_stack_depth = 127
)

type Stacks [__perf_max_stack_depth]uint64

var (
	__appender pyroscope.Appendable = nil
	once       sync.Once
)

func InitPyroscopeExporter(url string) error {
	var (
		err error
	)

	once.Do(func() {
		ip, _ := config.APISrvBindAddr()
		agentID := fmt.Sprintf(__defaultAgentIDFmt, ip)

		// make args
		ep := write.GetDefaultEndpointOptions()
		ep.URL = url
		glog.Infof("agentID:'%s', EndpointOptions: %s", agentID, litter.Sdump(ep))
		args := write.DefaultArguments()
		args.Endpoints = []*write.EndpointOptions{&ep}

		// make options
		options := component.Options{
			ID:         agentID,
			Logger:     log.With(__defaultLogger, "component", "pyrostub"),
			Registerer: prometheus.DefaultRegisterer,
			OnStateChange: func(e component.Exports) {
				__appender = e.(write.Exports).Receiver
				glog.Infof("agentID:'%s', set appender", agentID)
			},
		}

		if _, err = write.New(options, args); err != nil {
			err = errors.Wrapf(err, "agentID:'%s' new write component failed.", agentID)
		}
	})

	return err
}

type StackBuilder struct {
	stack []string
}

func (s *StackBuilder) Rest() {
	s.stack = s.stack[:0]
}

func (s *StackBuilder) Append(sym string) {
	s.stack = append(s.stack, sym)
}

func (s *StackBuilder) Len() int {
	return len(s.stack)
}

func (s *StackBuilder) Reverse() {
	lo.Reverse(s.stack)
}

func (s *StackBuilder) Stack() []string {
	return s.stack
}

func SubmitEBPFProfile(fromModule string, builders *pprof.ProfileBuilders) {
	bytesSend := 0
	for _, builder := range builders.Builders {
		buf := bytes.NewBuffer(nil)
		// 写入缓冲区
		if _, err := builder.Write(buf); err != nil {
			glog.Errorf("ebpf profile encode failed. err:%s", err.Error())
			continue
		}

		rawProfile := buf.Bytes()
		bytesSend += len(rawProfile)
		samples := []*pyroscope.RawSample{{RawProfile: rawProfile}}
		err := __appender.Appender().Append(context.Background(), builder.Labels, samples)
		if err != nil {
			glog.Errorf("eBPFProgram:'%s' ebpf profile append failed. err:%s", fromModule, err.Error())
			continue
		}
	}
	glog.Infof("eBPFProgram:'%s' submit epbf profiles done, send '%d' bytes", fromModule, bytesSend)
}

func WalkStack(stack []byte, pid int32, comm string, sb *StackBuilder) (int, error) {
	var (
		sym    *frame.Symbol
		stacks Stacks
		frames []string
		err    error
	)

	if len(stack) == 0 {
		return 0, errors.New("stack is empty")
	}

	if err := binary.Read(bytes.NewBuffer(stack), binary.LittleEndian, stacks[:__perf_max_stack_depth]); err != nil {
		return 0, errors.Wrap(err, "binary.Read data from stack")
	}

	resolveFailedCount := 0
	// for i := 0; i < len(stack); i += __stackFrameSize {
	// 	ip := binary.LittleEndian.Uint64(stack[i : i+__stackFrameSize])
	for _, pc := range stacks {
		if pc == 0 {
			break
		}
		// 解析 pc
		sym, err = frame.Resolve(pid, pc)
		if err == nil {
			if pid > 0 {
				symStr := fmt.Sprintf("%s+0x%x [%s]", sym.Name, sym.Offset, sym.Module)
				//symStr := fmt.Sprintf("%s [%s]", sym.Name, sym.Module)
				frames = append(frames, symStr)
			} else {
				frames = append(frames, sym.Name) //fmt.Sprintf("%s+0x%x", sym.Name, sym.Offset))
			}
		} else {
			if os.IsNotExist(errors.Cause(err)) {
				return 0, err
			}
			// 如果 error 是打开/proc/pid/maps 文件失败，那么不要继续了
			frames = append(frames, "[unknown]")
			//glog.Errorf("eBPFProgram:'%s' comm:'%s' err:%s", pp.name, comm, err.Error())
			resolveFailedCount++
		}
	}
	// 将堆栈反转
	lo.Reverse(frames)
	for _, f := range frames {
		sb.Append(f)
	}
	return resolveFailedCount, nil
}
