/*
 * @Author: CALM.WU
 * @Date: 2023-08-17 14:47:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-17 15:40:43
 */

package collector

import (
	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/grafana/pyroscope/ebpf/cpuonline"
	"github.com/pkg/errors"
	"golang.org/x/sys/unix"
)

// 1: invoke perf_event_open create fd for every cpu
// 2: attach ebpf program to every cpu

type peFDs []int

type perfEventLink_X struct {
	peFD     int
	useIoctl bool
	link     *link.RawLink // inherit from RawLink
}

func (pe_x *perfEventLink_X) Close() error {
	_ = unix.Close(pe_x.peFD)
	if !pe_x.useIoctl {
		pe_x.link.Close()
	}
	return nil
}

func OpenPerfEvents(pid int, sampleRate int) (peFDs, error) {
	var (
		fds        peFDs
		fd         int
		err        error
		onlineCPUs []uint
	)

	if onlineCPUs, err = cpuonline.Get(); err != nil {
		err = errors.Wrap(err, "get online cpus")
		return nil, err
	}

	attr := unix.PerfEventAttr{
		Type:   unix.PERF_TYPE_HARDWARE, //: 硬件事件，如CPU周期、缓存命中、分支错误等
		Config: unix.PERF_COUNT_HW_CPU_CYCLES,
		Sample: uint64(sampleRate), // 采样频率，每秒采样的次数
		Bits:   unix.PerfBitFreq,   // 表示是否使用采样频率而不是采样周期，默认为0，即使用采样周期
	}

	for _, cpu := range onlineCPUs {
		fd, err = unix.PerfEventOpen(&attr, pid, int(cpu), -1, unix.PERF_FLAG_FD_CLOEXEC)
		if err == nil {
			fds = append(fds, fd)
		} else {
			err = errors.Wrapf(err, "PerfEventOpen failed on cpu:%d", cpu)
			break
		}
	}

	if err != nil {
		for _, fd := range fds {
			unix.Close(fd)
		}
	}
	return fds, nil
}

func AttachPerfEvents(pefds peFDs, prog *ebpf.Program) ([]link.Link, error) {
	var links []link.Link

	return links, nil
}
