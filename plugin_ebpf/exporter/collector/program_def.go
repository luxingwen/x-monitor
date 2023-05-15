/*
 * @Author: CALM.WU
 * @Date: 2023-03-27 11:20:44
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-27 14:27:16
 */

package collector

import (
	"time"

	"github.com/cilium/ebpf/link"
	"github.com/sourcegraph/conc"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

const (
	cacheStateProgName  = "cachestat"
	runQLatencyProgName = "runqlatency"
	cpuSchedProgName    = "cpusched"
	processVMMProgName  = "processvm"
)

type eBPFProgramFilterCfg struct {
	ResScopeType config.XMInternalResourceType `mapstructure:"res_scope_type"` // 过滤的资源类型
	ResValue     int                           `mapstructure:"res_scope_val"`  // 过滤资源的值
}

type eBPFProgramFuncCfg struct {
	Name           string        `mapstructure:"name"` // 功能名
	MetricDesc     string        `mapstructure:"metric_desc"`
	GatherInterval time.Duration `mapstructure:"gather_interval"`
	ThresholdTime  time.Duration `mapstructure:"threshold_ms"`
}

type eBPFProgramExclude struct {
	Comms []string `mapstructure:"comms"`
}

func (ebp *eBPFProgramExclude) IsExcludeComm(comm string) bool {
	for _, c := range ebp.Comms {
		if c == comm {
			return true
		}
	}
	return false
}

type eBPFBaseProgram struct {
	// module
	name        string
	stopChan    chan struct{}
	wg          conc.WaitGroup
	gatherTimer *calmutils.Timer
	// ebpf
	links []link.Link
}
