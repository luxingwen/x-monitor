/*
 * @Author: CALM.WU
 * @Date: 2023-08-18 15:34:32
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-06 16:13:28
 */

package collector

import (
	"bytes"
	"context"
	"encoding/binary"
	"fmt"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/golang/glog"
	"github.com/grafana/agent/component/pyroscope"
	"github.com/grafana/pyroscope/ebpf/pprof"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/prometheus/model/labels"
	"github.com/samber/lo"
	"github.com/sanity-io/litter"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	bpfprog "xmonitor.calmwu/plugin_ebpf/exporter/internal/bpf_prog"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/eventcenter"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/pyrostub"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/symbols"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/utils"
)

const (
	__mapKey               uint32 = 0
	__perf_max_stack_depth        = 127
	__stackFrameSize              = (strconv.IntSize / 8)
	__defaultMetricName           = "__name__"
	labelServiceName              = "service_name"
	__defaultMatricValue          = "host"
)

type profileMatchTargetType uint32
type Stacks [__perf_max_stack_depth]uint64

const (
	MatchTargetNone profileMatchTargetType = iota
	MatchTargetComm
	MatchTargetPgID
	MatchTargetPidNS
	MatchTargetUnknown
)

func matchStr2Type(match string) profileMatchTargetType {
	switch match {
	case "MatchTargetComm":
		return MatchTargetComm
	case "MatchTargetPgID":
		return MatchTargetPgID
	case "MatchTargetPidNS":
		return MatchTargetPidNS
	case "MatchTargetNone":
		return MatchTargetNone
	}
	return MatchTargetUnknown
}

// profileTargetLabelsConf represents the configuration for a target label in a profile.
type profileTargetLabelsConf struct {
	LabelName string `mapstructure:"l_name"` // The name of the label.
	LabelVal  string `mapstructure:"l_val"`  // The value of the label.
}

// profileTargetConf represents the configuration for a profile target.
type profileTargetConf struct {
	Name   string                    `mapstructure:"name"`   // Name of the profile target.
	Match  string                    `mapstructure:"match"`  // Match condition for the profile target.
	Val    string                    `mapstructure:"val"`    // Value for the profile target.
	Labels []profileTargetLabelsConf `mapstructure:"labels"` // Labels for the profile target.
}

type ProfileTarget struct {
	Name                  string
	Labels                labels.Labels
	MatchType             profileMatchTargetType
	MatchVal              string
	fingerprintCalculated bool
	fingerprint           uint64
}

// Compare compares the given process information with the target's match value based on the match type.
// It returns true if the match is successful, otherwise false.
// The match type can be MatchTargetComm, MatchTargetPgID, or MatchTargetPidNS.
// If the match type is MatchTargetComm, it compares the process Name with the target's match value.
// If the match type is MatchTargetPgID, it compares the process group ID with the target's match value.
// If the match type is MatchTargetPidNS, it compares the process namespace ID with the target's match value.
func (pt *ProfileTarget) Compare(comm string, pgid int32, pidNS uint32) bool {
	switch pt.MatchType {
	case MatchTargetComm:
		return pt.MatchVal == comm
	case MatchTargetPgID:
		return pt.MatchVal == fmt.Sprintf("%d", pgid)
	case MatchTargetPidNS:
		return pt.MatchVal == fmt.Sprintf("%d", pidNS)
	default:
		return false
	}
}

func (pt *ProfileTarget) LSet() (uint64, labels.Labels) {
	if !pt.fingerprintCalculated {
		pt.fingerprint = pt.Labels.Hash()
		pt.fingerprintCalculated = true
	}

	return pt.fingerprint, pt.Labels
}

func newProfileTarget(targetConf *profileTargetConf) *ProfileTarget {
	MatchType := matchStr2Type(targetConf.Match)
	if MatchType == MatchTargetUnknown {
		glog.Errorf("Profile target'%s' match type:'%s' invalid.", targetConf.Name, targetConf.Match)
		return nil
	}

	labelSet := make(map[string]string, len(targetConf.Labels))

	// 指标名称
	labelSet[__defaultMetricName] = fmt.Sprintf("xm_%s_cpu", targetConf.Name)

	// 自定义标签
	for i := range targetConf.Labels {
		labelConf := &targetConf.Labels[i]
		labelSet[labelConf.LabelName] = labelConf.LabelVal
	}
	// service_name标签
	if targetConf.Name == __defaultMatricValue {
		labelSet[labelServiceName] = fmt.Sprintf("xm_host_%s", func() string {
			ip, _ := config.APISrvBindAddr()
			return ip
		}())
	} else {
		switch MatchType {
		case MatchTargetComm:
			// 固定标签，service_name = comm
			labelSet[labelServiceName] = targetConf.Name
		case MatchTargetPgID:
			// 固定标签，service_name = comm_pgid_xxx
			labelSet[labelServiceName] = fmt.Sprintf("%s_pgid_%s", targetConf.Name, targetConf.Val)
		case MatchTargetPidNS:
			// 固定标签，service_name = comm_pidns_xxx
			labelSet[labelServiceName] = fmt.Sprintf("%s_pidns_%s", targetConf.Name, targetConf.Val)
		}
	}

	return &ProfileTarget{
		Name:      targetConf.Name,
		Labels:    labels.FromMap(labelSet),
		MatchType: matchStr2Type(targetConf.Match),
		MatchVal:  targetConf.Val,
	}
}

type profileTargetFinderItf interface {
	FindTarget(comm string, pgid int32, pidNS uint32) *ProfileTarget
	UpdateTargets(targetConfs []profileTargetConf) error
	TargetCount() int
}

type profileTargetFinder struct {
	targets       []*ProfileTarget
	lock          sync.RWMutex
	defaultTarget *ProfileTarget
}

func NewProfileTargetFinder(targets []profileTargetConf) (profileTargetFinderItf, error) {
	ptf := &profileTargetFinder{}
	if err := ptf.UpdateTargets(targets); err != nil {
		return nil, errors.Wrap(err, "update targets failed.")
	}
	return ptf, nil
}

func (ptf *profileTargetFinder) FindTarget(comm string, pgid int32, pidNS uint32) *ProfileTarget {
	ptf.lock.RLock()
	defer ptf.lock.RUnlock()

	for _, target := range ptf.targets {
		if target.Compare(comm, pgid, pidNS) {
			return target
		}
	}
	return ptf.defaultTarget
}

func (ptf *profileTargetFinder) UpdateTargets(targetConfs []profileTargetConf) error {
	ptf.lock.Lock()
	defer ptf.lock.Unlock()

	targets := make([]*ProfileTarget, 0)

	for i := range targetConfs {
		targetConf := &targetConfs[i]
		target := newProfileTarget(targetConf)
		if target != nil {
			glog.Infof("add a new target:'%s'", litter.Sdump(*target))
			targets = append(targets, target)
		}
	}

	if ptf.defaultTarget == nil {
		ptf.defaultTarget = newProfileTarget(&profileTargetConf{
			Name:  __defaultMatricValue,
			Match: "MatchTargetNone",
		})
		glog.Infof("init default target:'%s'", litter.Sdump(*(ptf.defaultTarget)))
	}

	ptf.targets = nil
	ptf.targets = targets
	return nil
}

func (ptf *profileTargetFinder) TargetCount() int {
	ptf.lock.RLock()
	defer ptf.lock.RUnlock()

	return len(ptf.targets)
}

type profilePrivateArgs struct {
	SampleRate       int                 `mapstructure:"sample_rate"`
	PyroscopeSvrAddr string              `mapstructure:"pyroscope_server_addr"`
	GatherInterval   time.Duration       `mapstructure:"gather_interval"`
	Targets          []profileTargetConf `mapstructure:"targets"`
}

type profileProgram struct {
	*eBPFBaseProgram
	objs         *bpfmodule.XMProfileObjects
	perfLink     *bpfprog.PerfEvent
	pyroExporter pyroscope.Appendable
	tf           profileTargetFinderItf
}

type procStackInfo struct {
	pid         int32
	kStackBytes []byte
	uStackBytes []byte
	count       uint32
	comm        string
	PidNs       uint32
	Pgid        int32
	target      *ProfileTarget
}

var (
	__profileEBpfArgs    config.EBpfProgBaseArgs
	__profilePrivateArgs profilePrivateArgs

	__targetMatchTypeMap = map[string]profileMatchTargetType{
		"MatchTargetNone": MatchTargetNone,
		"MatchTargetComm": MatchTargetComm,
		"MatchTargetPgID": MatchTargetPgID,
	}
	_ profileTargetFinderItf = &profileTargetFinder{}

	ErrProfileDefaultTargetNotFound = errors.New("profile default target not found")
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

	if profileProg.tf, err = NewProfileTargetFinder(__profilePrivateArgs.Targets); err != nil {
		profileProg.finalizer()
		err = errors.Wrapf(err, "eBPFProgram:'%s' NewProfileTargetFinder failed.", name)
		return nil, err
	}
	glog.Infof("eBPFProgram:'%s' profileTarget count:%d", name, profileProg.tf.TargetCount())

	if profileProg.pyroExporter, err = pyrostub.NewPyroscopeExporter(__profilePrivateArgs.PyroscopeSvrAddr,
		prometheus.DefaultRegisterer); err != nil {
		profileProg.finalizer()
		err = errors.Wrapf(err, "eBPFProgram:'%s' NewPyroscopeExporter failed.", name)
		return nil, err
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

	defer func() {
		if err := recover(); err != nil {
			panicStack := calmutils.CallStack(3)
			glog.Errorf("Panic. err:%v, stack:%s", err, panicStack)
		}
	}()

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
					symbols.RemoveByPid(processExitEvt.Pid)
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
		profileSampleMapVal = bpfmodule.XMProfileXmProfileSampleData{}
		profileSampleCount  int
		profileStackMap     = pp.objs.XmProfileStackMap
		err                 error
		psis                []*procStackInfo
		sym                 *symbols.Symbol
	)

	// 收集xm_profile_sample_count_map数据
	keys := make([]bpfmodule.XMProfileXmProfileSample, 0)
	vals := make([]bpfmodule.XMProfileXmProfileSampleData, 0)

	// 批量获取数据
	it := profileSampleMap.Iterate()
	for it.Next(&profileSampleMapKey, &profileSampleMapVal) {
		keys = append(keys, profileSampleMapKey)
		vals = append(vals, profileSampleMapVal)
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
		sampleInfo := &keys[i]
		sampleData := &vals[i]

		stackIDSet[sampleInfo.UserStackId] = struct{}{}
		stackIDSet[sampleInfo.KernelStackId] = struct{}{}

		psi := &procStackInfo{
			pid:         sampleInfo.Pid,
			count:       sampleData.Count,
			PidNs:       sampleData.PidNs,
			Pgid:        sampleData.Pgid,
			comm:        utils.CommToString(sampleInfo.Comm[:]),
			uStackBytes: pfnGetStackBytes(sampleInfo.UserStackId),
			kStackBytes: pfnGetStackBytes(sampleInfo.KernelStackId),
			target:      pp.tf.FindTarget(utils.CommToString(sampleInfo.Comm[:]), sampleData.Pgid, sampleData.PidNs),
		}

		psis = append(psis, psi)
	}

	pfnWalkStack := func(stack []byte, pid int32, comm string, sb *stackBuilder) {
		if len(stack) == 0 {
			return
		}

		var stacks Stacks
		binary.Read(bytes.NewBuffer(stack), binary.LittleEndian, stacks[:__perf_max_stack_depth])

		var frames []string
		// for i := 0; i < len(stack); i += __stackFrameSize {
		// 	ip := binary.LittleEndian.Uint64(stack[i : i+__stackFrameSize])
		for _, ip := range stacks {
			if ip == 0 {
				break
			}
			// 解析ip
			sym, err = symbols.Resolve(pid, ip)
			if err == nil {
				if pid > 0 {
					symStr := fmt.Sprintf("%s+0x%x [%s]", sym.Name, sym.Offset, sym.Module)
					//glog.Infof("eBPFProgram:'%s' comm:'%s' ip:0x%x ==> '%s'", pp.name, comm, ip, symStr)
					frames = append(frames, symStr)
				} else {
					frames = append(frames, sym.Name) //fmt.Sprintf("%s+0x%x", sym.Name, sym.Offset))
				}
			} else {
				frames = append(frames, "[unknown]")
				glog.Errorf("eBPFProgram:'%s' comm:'%s' err:%s", pp.name, comm, err.Error())
			}
		}
		// 将堆栈反转
		lo.Reverse(frames)
		for _, f := range frames {
			sb.append(f)
		}
	}

	builders := pprof.NewProfileBuilders(__profilePrivateArgs.SampleRate)
	// 解析堆栈
	sb := stackBuilder{}
	for _, psi := range psis {
		sb.rest()
		sb.append(psi.comm)
		pfnWalkStack(psi.uStackBytes, psi.pid, psi.comm, &sb)
		pfnWalkStack(psi.kStackBytes, 0, "kernel", &sb)

		if len(sb.stack) == 1 {
			continue
		}
		lo.Reverse(sb.stack)

		lablesHash, labels := psi.target.LSet()
		builder := builders.BuilderForTarget(lablesHash, labels)
		builder.AddSample(sb.stack, uint64(psi.count))

		glog.Infof("eBPFProgram:'%s' comm:'%s', pid:%d count:%d build depth:%d stacks:\n\t%s", pp.name, psi.comm, psi.pid, psi.count, len(sb.stack), strings.Join(sb.stack, "\n\t"))
	}
	// 上报ebpf profile结果到pyroscope
	pp.submitEBPFProfile(builders)

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

	glog.Infof("eBPFProgram:'%s' symCache size:%d", pp.name, symbols.Size())
}

func (pp *profileProgram) submitEBPFProfile(builders *pprof.ProfileBuilders) {
	//glog.Infof("eBPFProgram:'%s' start submitEBPFProfile, profiles count:%d", pp.name, len(builders.Builders))

	bytesSend := 0
	for _, builder := range builders.Builders {
		buf := bytes.NewBuffer(nil)
		// 写入缓冲区
		if _, err := builder.Write(buf); err != nil {
			glog.Errorf("eBPFProgram:'%s' ebpf profile encode failed. err:%s", pp.name, err.Error())
			continue
		}

		rawProfile := buf.Bytes()
		bytesSend += len(rawProfile)
		samples := []*pyroscope.RawSample{{RawProfile: rawProfile}}
		err := pp.pyroExporter.Appender().Append(context.Background(), builder.Labels, samples)
		if err != nil {
			glog.Errorf("eBPFProgram:'%s' ebpf profile append failed. err:%s", pp.name, err.Error())
			continue
		}
	}
	glog.Infof("eBPFProgram:'%s' submit epbf profiles done, send '%d' bytes", pp.name, bytesSend)
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