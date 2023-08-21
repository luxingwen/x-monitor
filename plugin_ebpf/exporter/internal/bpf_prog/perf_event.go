/*
 * @Author: CALM.WU
 * @Date: 2023-08-18 14:48:42
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 14:49:04
 */

package bpfprog

import (
	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/golang/glog"
	"github.com/grafana/pyroscope/ebpf/cpuonline"
	"github.com/pkg/errors"
	"golang.org/x/sys/unix"
)

// 1: invoke perf_event_open create fd for every cpu
// 2: attach ebpf program to every cpu

type peFDs []int

type perfEventLink struct {
	link *link.RawLink
	peFD int
}

func createPerfEventLink(pid int, sampleRate int, cpu int, prog *ebpf.Program) (*perfEventLink, error) {
	var err error

	pel := new(perfEventLink)
	if err = pel.openPerfEvent(pid, cpu, uint64(sampleRate)); err != nil {
		return nil, errors.Wrap(err, "openPerfEvent failed.")
	} else {
		if err = pel.attachPerfEvent(prog); err != nil {
			glog.Warningf("attach perfEvent failed: %s. so change to use ioctl", err)
			err = pel.attachPerfEventIoctl(prog)
			if err != nil {
				glog.Errorf("attach perfEvent use Ioctl failed: %s", err)
			}
		}
	}

	if err != nil {
		pel = nil
		err = errors.Wrap(err, "attachPerfEvent failed")
		return nil, err
	}

	return pel, nil
}

func (pel *perfEventLink) openPerfEvent(pid int, cpu int, sampleRate uint64) error {
	attr := unix.PerfEventAttr{
		// Type:   unix.PERF_TYPE_HARDWARE,       //,
		// Config: unix.PERF_COUNT_HW_CPU_CYCLES, //,
		Type:   unix.PERF_TYPE_SOFTWARE,
		Config: unix.PERF_COUNT_SW_CPU_CLOCK,
		Sample: sampleRate,       // 采样频率，每秒采样的次数
		Bits:   unix.PerfBitFreq, // 表示是否使用采样频率而不是采样周期，默认为0，即使用采样周期
	}

	if fd, err := unix.PerfEventOpen(&attr, pid, cpu, -1, unix.PERF_FLAG_FD_CLOEXEC); err != nil {
		return errors.Wrapf(err, "open PerfEvent on pid:%d, cpu:%d failed.", pid, cpu)
	} else {
		pel.peFD = fd
		glog.Infof("open PerfEvent on pid:%d, cpu:%d succeeded, get fd:%d", pid, cpu, fd)
	}

	return nil
}

func (pel *perfEventLink) attachPerfEvent(prog *ebpf.Program) error {
	var err error

	opts := link.RawLinkOptions{
		Target:  pel.peFD,
		Program: prog,
		Attach:  ebpf.AttachPerfEvent,
	}

	if pel.link, err = link.AttachRawLink(opts); err != nil {
		err = errors.Wrap(err, "attach RawLink failed.")
		return err
	} else {
		pel.link = nil
		glog.Infof("attach RawLink on fd:%d succeeded", pel.peFD)
	}

	return nil
}

func (pel *perfEventLink) attachPerfEventIoctl(prog *ebpf.Program) error {
	// Assign the eBPF program to the perf event.
	err := unix.IoctlSetInt(pel.peFD, unix.PERF_EVENT_IOC_SET_BPF, prog.FD())
	if err != nil {
		return errors.Wrap(err, "ioctl setting PerfEvent program failed")
	}

	// PERF_EVENT_IOC_ENABLE and _DISABLE ignore their given values.
	if err := unix.IoctlSetInt(pel.peFD, unix.PERF_EVENT_IOC_ENABLE, 0); err != nil {
		return errors.Wrap(err, "ioctl enable PerfEvent failed")
	}
	glog.Infof("use ioctl assign ebpf.Program to PerfEvent succeeded, fd:%d", pel.peFD)

	return nil
}

func (pel *perfEventLink) Close() {
	_ = unix.Close(pel.peFD)
	if pel.link != nil {
		pel.link.Close()
	}
}

//------------------------------------------------------------

type PerfEvent struct {
	links      []*perfEventLink
	pid        int
	sampleRate int
}

func AttachPerfEventProg(pid int, sampleRate int, prog *ebpf.Program) (*PerfEvent, error) {
	var (
		onlineCPUs []uint
		err        error
		pel        *perfEventLink
	)

	if prog == nil {
		return nil, errors.New("attach prog is nil!!")
	}

	if prog.FD() < 0 {
		return nil, errors.Wrap(unix.EBADF, "attach prog is invalid!!!")
	}

	pe := &PerfEvent{
		pid:        pid,
		sampleRate: sampleRate,
	}

	if onlineCPUs, err = cpuonline.Get(); err != nil {
		return nil, errors.Wrap(err, "get online cpus")
	}

	glog.Infof("onlineCpu: %#v", onlineCPUs)

	for _, cpu := range onlineCPUs {
		if pel, err = createPerfEventLink(pid, sampleRate, int(cpu), prog); err == nil {
			pe.links = append(pe.links, pel)
		} else {
			err = errors.Wrapf(err, "createPerfEventLink failed on cpu:%d", cpu)
			break
		}
	}

	if err != nil {
		pe.Detach()
		return nil, err
	}

	glog.Infof("AttachPerfEventProg success, pid:%d, sampleRate:%d", pid, sampleRate)

	return pe, nil
}

func (pe *PerfEvent) Detach() {
	for _, pel := range pe.links {
		pel.Close()
	}
}
