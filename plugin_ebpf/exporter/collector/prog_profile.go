/*
 * @Author: CALM.WU
 * @Date: 2023-08-18 15:34:32
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 16:17:38
 */

package collector

import (
	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	bpfprog "xmonitor.calmwu/plugin_ebpf/exporter/internal/bpf_prog"
)

type profileProgram struct {
	*eBPFBaseProgram
	objs *bpfmodule.XMProfileObjects
	pe   *bpfprog.PerfEvent
}

func init() {
	registerEBPFProgram(profileProgName, newProfileProgram)
}

func newProfileProgram(name string) (eBPFProgram, error) {
	return nil, nil
}

func (pp *profileProgram) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (pp *profileProgram) handlingeBPFData() {
	glog.Infof("eBPFProgram:'%s' start handling eBPF Data...", pp.name)
}

func (pp *profileProgram) Stop() {

}

func (pp *profileProgram) Reload() error {
	return nil
}
