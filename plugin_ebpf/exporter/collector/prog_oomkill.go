/*
 * @Author: CALM.WU
 * @Date: 2023-06-14 10:31:37
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2023-06-14 10:31:37
 */

package collector

import (
	"github.com/cilium/ebpf/ringbuf"
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

type oomKillPrograms struct {
	*eBPFBaseProgram
	oomKillEvtRD       *ringbuf.Reader
	objs               *bpfmodule.XMOomKillObjects
	oomKillEvtDataChan *bpfmodule.OomkillEvtDataChannel
}

func init() {
	registerEBPFProgram(oomKillProgName, newOomKillProgram)
}

func newOomKillProgram(name string) (eBPFProgram, error) {
	prog := &oomKillPrograms{}

	return prog, nil
}

func (okp *oomKillPrograms) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (okp *oomKillPrograms) Stop() {
	if okp.oomKillEvtRD != nil {
		okp.oomKillEvtRD.Close()
	}

	okp.stop()
}
