/*
 * @Author: CALM.WU
 * @Date: 2023-08-18 15:34:32
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-04 16:59:58
 */

package collector

import (
	"encoding/binary"
	"fmt"
	"strconv"
	"strings"
	"time"

	"github.com/golang/glog"
	"github.com/grafana/agent/component/pyroscope"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/samber/lo"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	bpfprog "xmonitor.calmwu/plugin_ebpf/exporter/internal/bpf_prog"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/eventcenter"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/symbols"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/utils"
)

const (
	__mapKey               uint32 = 0
	__perf_max_stack_depth        = 127
	__stackFrameSize              = (strconv.IntSize / 8)
)

type profilePrivateArgs struct {
	SampleRate       int           `mapstructure:"sample_rate"`
	PyroscopeAppName string        `mapstructure:"pyroscope_application_name"`
	PyroscopeSvrAddr string        `mapstructure:"pyroscope_server_addr"`
	GatherInterval   time.Duration `mapstructure:"gather_interval"`
}

type profileProgram struct {
	*eBPFBaseProgram
	objs           *bpfmodule.XMProfileObjects
	perfLink       *bpfprog.PerfEvent
	pyroAppendable pyroscope.Appendable
}

type procStackInfo struct {
	pid         int32
	kStackBytes []byte
	uStackBytes []byte
	count       uint32
	comm        string
}

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
		objs: new(bpfmodule.XMProfileObjects),
	}

	// 加载eBPF对象
	if err = bpfmodule.LoadXMProfileObjects(profileProg.objs, nil); err != nil {
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
					glog.Infof("eBPFProgram:'%s', receive eBPF Event'%s'", pp.name, litter.Sdump(eBPFEvtInfo))
					processExitEvt := eBPFEvtInfo.EvtData.(*eventcenter.EBPFEventProcessExit)
					symbols.RemoveByPId(processExitEvt.Pid)
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
	defer glog.Infof("eBPFProgram:'%s' collect Profiles completed", pp.name)

	var (
		profileSampleMap    = pp.objs.XmProfileSampleCountMap
		profileSampleMapKey = bpfmodule.XMProfileXmProfileSample{}
		profileSampleMapVal = uint32(0)
		profileSampleCount  int
		profileStackMap     = pp.objs.XmProfileStackMap
		err                 error
		psis                []*procStackInfo
		sym                 *symbols.Symbol
	)

	// 收集xm_profile_sample_count_map数据
	keys := make([]bpfmodule.XMProfileXmProfileSample, 0)
	counts := make([]uint32, 0)

	// 批量获取数据
	it := profileSampleMap.Iterate()
	for it.Next(&profileSampleMapKey, &profileSampleMapVal) {
		keys = append(keys, profileSampleMapKey)
		counts = append(counts, profileSampleMapVal)
		profileSampleCount++
	}

	if err = it.Err(); err != nil {
		glog.Errorf("eBPFProgram:'%s' get profileSampleMap iterator failed, err:%s", pp.name, err.Error())
		return
	}

	glog.Infof("eBPFProgram:'%s' get profileSampleMap iterator succeeded, keyCount:%d", pp.name, profileSampleCount)

	stackIDSet := map[int32]struct{}{}

	pfnGetStackBytes := func(stackID int32) []byte {
		if stackID > 0 {
			stackBytes, err := profileStackMap.LookupBytes(stackID)
			if err != nil {
				glog.Errorf("eBPFProgram:'%s' stackMap.LookupBytes('stackID=%d)' failed, err:%s", pp.name, stackID, err.Error())
				return nil
			}
			return stackBytes
		}
		return nil
	}

	// 获取堆栈
	for i := range keys {
		// XMProfileXmProfileSample
		sampleInfo := keys[i]
		sampleCount := counts[i]

		stackIDSet[sampleInfo.UserStackId] = struct{}{}
		stackIDSet[sampleInfo.KernelStackId] = struct{}{}

		psi := &procStackInfo{
			pid:         sampleInfo.Pid,
			count:       sampleCount,
			comm:        utils.CommToString(sampleInfo.Comm[:]),
			uStackBytes: pfnGetStackBytes(sampleInfo.UserStackId),
			kStackBytes: pfnGetStackBytes(sampleInfo.KernelStackId),
		}

		psis = append(psis, psi)
	}

	pfnWalkStack := func(stack []byte, pid int32, sb *stackBuilder) {
		if len(stack) == 0 {
			return
		}

		var frames []string
		for i := 0; i < len(stack); i += __stackFrameSize {
			ip := binary.LittleEndian.Uint64(stack[i : i+__stackFrameSize])
			if ip == 0 {
				break
			}
			// 解析ip
			sym, err = symbols.Resolve(pid, ip)
			if err == nil {
				if pid > 0 {
					frames = append(frames, fmt.Sprintf("%s+0x%x [%s]", sym.Name, sym.Offset, sym.Module))
				} else {
					frames = append(frames, fmt.Sprintf("%s+0x%x", sym.Name, sym.Offset))
				}
			} else {
				frames = append(frames, "[unknown]")
			}
		}
		// 将堆栈反转
		lo.Reverse(frames)
		for _, f := range frames {
			sb.append(f)
		}
	}

	// 解析堆栈
	sb := stackBuilder{}
	for _, psi := range psis {
		sb.rest()
		sb.append(psi.comm)
		pfnWalkStack(psi.uStackBytes, psi.pid, &sb)
		pfnWalkStack(psi.kStackBytes, 0, &sb)

		if len(sb.stack) == 0 {
			continue
		}
		lo.Reverse(sb.stack)

		glog.Infof("eBPFProgram:'%s' comm:'%s', pid:%d, count:%d stacks:\n\t%s",
			pp.name, psi.comm, psi.pid, psi.count, strings.Join(sb.stack, "\n\t"))
	}

	// 清理profileSample map
	for i := range keys {
		if err = profileSampleMap.Delete(keys[i]); err != nil {
			glog.Infof("eBPFProgram:'%s' sampleMap.Delete('key=%#v)' failed, err:%s", pp.name, keys[i], err.Error())
		}
	}

	// 清理stack map
	for stackID := range stackIDSet {
		if stackID > 0 {
			if err = profileStackMap.Delete(stackID); err != nil {
				glog.Errorf("eBPFProgram:'%s' stackMap.Delete('stackID=%d)' failed, err:%s", pp.name, stackID, err.Error())
			}
		}
	}

	glog.Infof("eBPFProgram:'%s' symCache size:%d", symbols.Size())
}

type stackBuilder struct {
	stack []string
}

func (s *stackBuilder) rest() {
	s.stack = s.stack[:0]
}

func (s *stackBuilder) append(sym string) {
	s.stack = append(s.stack, sym)
}
