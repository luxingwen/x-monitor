/*
 * @Author: CALM.WU
 * @Date: 2023-06-14 10:31:37
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2023-06-14 10:31:37
 */

package collector

import (
	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

const (
	defaultOomKillEvtChanSize = ((1 << 2) << 10)
)

type oomKillProgram struct {
	*eBPFBaseProgram
	oomKillEvtRD       *ringbuf.Reader
	objs               *bpfmodule.XMOomKillObjects
	oomKillEvtDataChan *bpfmodule.OomkillEvtDataChannel
}

func init() {
	registerEBPFProgram(oomKillProgName, newOomKillProgram)
}

func loadToRunOomKillProg(name string, prog *oomKillProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMOomKillObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMOomKill, func(spec *ebpf.CollectionSpec) error {
		return nil
	})

	if err != nil {
		prog.objs.Close()
		prog.objs = nil
		return err
	}

	prog.oomKillEvtRD, err = ringbuf.NewReader(prog.objs.XmOomkillEventRingbufMap)
	if err != nil {
		for _, link := range prog.links {
			link.Close()
		}
		prog.links = nil
		prog.objs.Close()
		prog.objs = nil
	}
	return err
}

func newOomKillProgram(name string) (eBPFProgram, error) {
	prog := &oomKillProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:     name,
			stopChan: make(chan struct{}),
		},
		oomKillEvtDataChan: bpfmodule.NewOomkillEvtDataChannel(defaultOomKillEvtChanSize),
	}

	if err := loadToRunOomKillProg(name, prog); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' loadToRunOomKillProg failed.", name)
		glog.Error(err)
		return nil, err
	}

	return prog, nil
}

func (okp *oomKillProgram) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (okp *oomKillProgram) Stop() {
	if okp.oomKillEvtRD != nil {
		okp.oomKillEvtRD.Close()
	}

	okp.stop()

	if okp.objs != nil {
		okp.objs.Close()
		okp.objs = nil
	}

	okp.oomKillEvtDataChan.SafeClose()
	glog.Infof("eBPFProgram:'%s' stopped", okp.name)
}
