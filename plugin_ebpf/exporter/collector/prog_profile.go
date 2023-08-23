/*
 * @Author: CALM.WU
 * @Date: 2023-08-18 15:34:32
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 16:17:38
 */

package collector

import (
	"time"

	"github.com/golang/glog"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	bpfprog "xmonitor.calmwu/plugin_ebpf/exporter/internal/bpf_prog"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/eventcenter"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/symbols"
)

type profilePrivateArgs struct {
	SampleRate       int           `mapstructure:"sample_rate"`
	PyroscopeAppName string        `mapstructure:"pyroscope_application_name"`
	PyroscopeSvrAddr string        `mapstructure:"pyroscope_server_addr"`
	GatherInterval   time.Duration `mapstructure:"gather_interval"`
}

type profileProgram struct {
	*eBPFBaseProgram
	objs     *bpfmodule.XMProfileObjects
	perfLink *bpfprog.PerfEvent
}

const (
	__mapKey uint32 = 0
)

var (
	__profileEBpfArgs    config.EBpfProgBaseArgs
	__profilePrivateArgs profilePrivateArgs
)

func init() {
	registerEBPFProgram(profileProgName, newProfileProgram)
}

func newProfileProgram(name string) (eBPFProgram, error) {
	var err error

	// unmarshal config
	mapstructure.Decode(config.ProgramConfigByName(name).Args.EBpfProg, &__profileEBpfArgs)
	mapstructure.Decode(config.ProgramConfigByName(name).Args.Private, &__profilePrivateArgs)
	__profilePrivateArgs.GatherInterval = __profilePrivateArgs.GatherInterval * time.Second

	glog.Infof("eBPFProgram:'%s' EbpfArgs:%s, PrivateArgs:%s", name, litter.Sdump(__profileEBpfArgs),
		litter.Sdump(__profilePrivateArgs))

	profileProg := &profileProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
	}

	// 加载eBPF对象
	if err = bpfmodule.LoadXMProfileObjects(&profileProg.objs, nil); err != nil {
		profileProg.finalizer()
		err = errors.Wrapf(err, "eBPFProgram:'%s' LoadXMProfileObjects failed.", name)
		return nil, err
	}

	// 设置ebpf prog参数
	args := new(bpfmodule.XMProfileXmProgFilterArgs)
	args.ScopeType = bpfmodule.XMProfileXmProgFilterTargetScopeType(__profileEBpfArgs.FilterScopeType)
	args.FilterCondValue = uint64(__profileEBpfArgs.FilterScopeValue)

	if err := profileProg.objs.XmProfileArgMap.Update(__mapKey, args, 0); err != nil {
		profileProg.finalizer()
		err = errors.Wrapf(err, "eBPFProgram:'%s' update profile args map failed.", name)
		return nil, err
	}

	// attach eBPF程序
	profileProg.perfLink, err = bpfprog.AttachPerfEventProg(-1, __profilePrivateArgs.SampleRate, profileProg.objs.XmDoPerfEvent)
	if err != nil {
		profileProg.Stop()
		err = errors.Wrapf(err, "eBPFProgram:'%s' AttachPerfEventProg failed.", name)
		return nil, err
	}

	// start handling eBPF data
	profileProg.wg.Go(profileProg.handlingeBPFData)

	return profileProg, nil
}

func (pp *profileProgram) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (pp *profileProgram) handlingeBPFData() {
	glog.Infof("eBPFProgram:'%s' start handling eBPF Data...", pp.name)

	// 上报定时器
	pp.gatherTimer.Reset(__profilePrivateArgs.GatherInterval)
	eBPFEventReadChan := eventcenter.DefInstance.Subscribe(pp.name, eventcenter.EBPF_EVENT_PROCESS_EXIT)

loop:
	for {
		select {
		case <-pp.stopChan:
			glog.Warningf("eBPFProgram:'%s' handling eBPF Data goroutine receive stop notify", pp.name)
			break loop
		case <-pp.gatherTimer.Chan():
			// 收集堆栈，上报pyroscope，清理
			pp.collectProfiles()
			pp.gatherTimer.Reset(__profilePrivateArgs.GatherInterval)
		case eBPFEvtInfo, ok := <-eBPFEventReadChan.C:
			if ok {
				switch eBPFEvtInfo.EvtType {
				case eventcenter.EBPF_EVENT_PROCESS_EXIT:
					processExitEvt := eBPFEvtInfo.EvtData.(*eventcenter.EBPFEventProcessExit)
					symbols.RemovePidCache(uint32(processExitEvt.Pid))
				}
				eBPFEvtInfo = nil
			}
		}
	}
}

func (pp *profileProgram) Stop() {
	pp.finalizer()

	pp.perfLink.Detach()

	if pp.objs != nil {
		pp.objs.Close()
		pp.objs = nil
	}
	glog.Infof("eBPFProgram:'%s' stopped", pp.name)
}

func (pp *profileProgram) Reload() error {
	return nil
}

func (pp *profileProgram) collectProfiles() {
	glog.Infof("eBPFProgram:'%s' start collect Profiles", pp.name)

	var (
		profileSampleMap = pp.objs.XmProfileSampleCountMap
		mapMaxSize       = profileSampleMap.MaxEntries()
		nextKey          = bpfmodule.XMProfileXmProfileSample{}
		keyCount         int
		err              error
	)

	// 收集xm_profile_sample_count_map数据
	keys := make([]bpfmodule.XMProfileXmProfileSample, mapMaxSize)
	counts := make([]uint32, mapMaxSize)

	// 批量获取数据
	keyCount, err = profileSampleMap.BatchLookupAndDelete(nil, &nextKey, keys, counts, nil)
	if err != nil {
		glog.Errorf("eBPFProgram:'%s' BatchLookupAndDelete failed, err:%s", pp.name, err.Error())
		return
	}

	glog.Infof("eBPFProgram:'%s' BatchLookupAndDelete keyCount:%d", pp.name, keyCount)

	keys = keys[:keyCount]
	counts = counts[:keyCount]

	for i := range keys {
		// XMProfileXmProfileSample
		sampleInfo := keys[i]
		sampleCount := counts[i]

		glog.Infof("sampleInfo:%s, sampleCount:%d", litter.Sdump(sampleInfo), sampleCount)
	}
}
