/*
 * @Author: CALM.WU
 * @Date: 2023-03-25 12:43:28
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-25 15:10:56
 */

package collector

import (
	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

type cpuSchedProgram struct {
	*eBPFBaseProgram
	// eBPF对象
	objs *bpfmodule.XMCpuScheduleObjects
	// cpu sched evt data channel
	cpuSchedEvtDataChan *bpfmodule.CpuSchedEvtDataChannel
}

const (
	defaultCpuSchedEvtChanSize = 128
)

func init() {
	registerEBPFProgram(cpuSchedProgName, newCpuSchedProgram)
}

func newCpuSchedProgram(name string) (eBPFProgram, error) {
	ebpfProg := &cpuSchedProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
		cpuSchedEvtDataChan: bpfmodule.NewCpuSchedEvtDataChannel(defaultCpuSchedEvtChanSize),
	}

	return ebpfProg, nil
}

func (csp *cpuSchedProgram) Update(ch chan<- prometheus.Metric) error {

	// 将cpu schedule event数据转换为Prometheus指标
L:
	for {
		select {
		case cpuSchedEvtData, ok := <-csp.cpuSchedEvtDataChan.C:
			if ok {
				glog.Infof("eBPFProgram:'%s' cpuSchedEvtData:%s", csp.name, litter.Sdump(cpuSchedEvtData))
			}
		default:
			break L
		}
	}
	return nil
}

func (csp *cpuSchedProgram) Stop() {
	csp.cpuSchedEvtDataChan.SafeClose()
	glog.Infof("eBPFProgram:'%s' stopped.", csp.name)
}
