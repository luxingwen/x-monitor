/*
 * @Author: CALM.WU
 * @Date: 2023-04-27 14:10:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-27 15:21:07
 */

package collector

import (
	"bytes"
	"encoding/binary"
	"time"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal"
)

// processVMMProgRodata is used to filter the resources of the specified type
// and value in the rodata section of the process vmmap
type processVMMProgRodata struct {
	FilterScopeType  config.XMInternalResourceType `mapstructure:"filter_scope_type"`  // 过滤的资源类型
	FilterScopeValue int                           `mapstructure:"filter_scope_value"` // 过滤资源的值
}

type processVMMProgram struct {
	*eBPFBaseProgram

	roData         processVMMProgRodata
	gatherInterval time.Duration

	vmmEvtRD *ringbuf.Reader
	objs     *bpfmodule.XMProcessVMMObjects
}

func init() {
	registerEBPFProgram(processVMMProgName, newProcessVMMProgram)
}

func loadToRunProcessVMAProg(name string, prog *processVMMProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMProcessVMMObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMProcessVMM, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_scope_type":  int32(0),
			"__filter_scope_value": int64(0),
		})

		if err != nil {
			return errors.Wrapf(err, "eBPFProgram:'%s'", name)
		}
		return nil
	})

	if err != nil {
		prog.objs.Close()
		prog.objs = nil
		return err
	}

	prog.vmmEvtRD, err = ringbuf.NewReader(prog.objs.XmVmaEventRingbufMap)
	if err != nil {
		for _, link := range prog.links {
			link.Close()
		}
		prog.links = nil
		prog.objs.Close()
		prog.objs = nil
	}

	prog.wg.Go(prog.tracingProcessVMAEvent)
	return err
}

func newProcessVMMProgram(name string) (eBPFProgram, error) {
	prog := &processVMMProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
	}

	if err := loadToRunProcessVMAProg(name, prog); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' runProcessVMAProgram failed.", name)
		glog.Error(err)
		return nil, err
	}
	return prog, nil
}

func (pvp *processVMMProgram) tracingProcessVMAEvent() {
	glog.Infof("eBPFProgram:'%s' start tracing process vma event data...", pvp.name)

loop:
	for {
		record, err := pvp.vmmEvtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningf("eBPFProgram:'%s' tracing process vma event receive stop notify", pvp.name)
				break loop
			}
			glog.Errorf("eBPFProgram:'%s' Read error. err:%s", pvp.name, err.Error())
			continue
		}

		processVMAEvtData := new(bpfmodule.XMProcessVMMXmVmmEvtData)
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, processVMAEvtData); err != nil {
			glog.Errorf("failed to parse XMProcessVMAXmVmaEvtData, err: %v", err)
			continue
		}

		comm := internal.CommToString(processVMAEvtData.Comm[:])
		glog.Infof("eBPFProgram:'%s' tgid:%d, pid:%d, comm:'%s' expand VA space by '%d' bytes",
			pvp.name, processVMAEvtData.Tgid, processVMAEvtData.Pid, comm, processVMAEvtData.Len)
	}
}

func (pvp *processVMMProgram) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (pvp *processVMMProgram) Stop() {
	if pvp.vmmEvtRD != nil {
		pvp.vmmEvtRD.Close()
	}

	pvp.stop()

	if pvp.objs != nil {
		pvp.objs.Close()
		pvp.objs = nil
	}

	glog.Infof("eBPFProgram:'%s' stopped", pvp.name)
}
