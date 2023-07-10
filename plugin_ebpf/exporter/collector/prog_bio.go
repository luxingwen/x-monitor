/*
 * @Author: CALM.WU
 * @Date: 2023-07-10 14:15:12
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-07-10 15:50:27
 */

package collector

import (
	"sync"
	"time"

	"github.com/cilium/ebpf"
	"github.com/golang/glog"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

type bioProgRodata struct {
	FilterPerCmdFlag bool `mapstructure:"filter_per_cmd_flag"`
}

type bioProgram struct {
	*eBPFBaseProgram
	gatherInterval time.Duration
	objs           *bpfmodule.XMBioObjects
	guard          sync.Mutex
}

var (
	bioRoData bioProgRodata
)

func init() {
	registerEBPFProgram(bioProgName, newBIOProgram)
}

func loadToRunBIOProg(name string, prog *bioProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMBioObjects)
	prog.links, err = attatchToRun(name, prog.objs, bpfmodule.LoadXMBio, func(spec *ebpf.CollectionSpec) error {
		err = spec.RewriteConstants(map[string]interface{}{
			"__filter_per_cmd_flag": bioRoData.FilterPerCmdFlag,
		})

		if err != nil {
			return errors.Wrap(err, "RewriteConstants failed.")
		}
		return err
	})

	if err != nil {
		prog.objs.Close()
		prog.objs = nil
		return err
	}

	return err
}

func newBIOProgram(name string) (eBPFProgram, error) {
	bioProg := &bioProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
		gatherInterval: config.ProgramConfigByName(name).GatherInterval * time.Second,
	}

	mapstructure.Decode(config.ProgramConfigByName(name).Filter.ProgRodata, &bioRoData)
	glog.Infof("eBPFProgram:'%s' bioRoData:%s", name, litter.Sdump(bioRoData))

	if err := loadToRunBIOProg(name, bioProg); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' load and run failed.", name)
		glog.Error(err)
		return nil, err
	}

	bioProg.wg.Go(bioProg.handingeBPFData)

	return bioProg, nil
}

func (bp *bioProgram) handingeBPFData() {
	var bioInfoMapKey bpfmodule.XMBioXmBioKey
	var bioInfoMapData bpfmodule.XMBioXmBioData

	glog.Infof("eBPFProgram:'%s' start handling eBPF Data...", bp.name)

	bp.gatherTimer.Reset(bp.gatherInterval)

loop:
	for {
		select {
		case <-bp.stopChan:
			glog.Warningf("eBPFProgram:'%s' handling eBPF Data goroutine receive stop notify", bp.name)
			break loop
		case <-bp.gatherTimer.Chan():
			entries := bp.objs.XmBioInfoMap.Iterate()
			for entries.Next(&bioInfoMapKey, &bioInfoMapData) {
				glog.Infof("eBPFProgram:'%s' key:%s, data:%#v", bp.name, litter.Sdump(bioInfoMapKey), bioInfoMapData)
			}

			if err := entries.Err(); err != nil {
				glog.Errorf("eBPFProgram:'%s' iterator BioInfoMap failed, err:%s", err.Error())
			} else {
				// 开始计算bio采集数据
			}

			// 重置定时器
			bp.gatherTimer.Reset(bp.gatherInterval)
		}
	}
}

func (bp *bioProgram) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (bp *bioProgram) Stop() {
	bp.stop()

	if bp.objs != nil {
		bp.objs.Close()
		bp.objs = nil
	}
	glog.Infof("eBPFProgram:'%s' stopped", bp.name)
}
