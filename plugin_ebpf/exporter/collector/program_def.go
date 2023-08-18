/*
 * @Author: CALM.WU
 * @Date: 2023-03-27 11:20:44
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 16:14:04
 */

package collector

import (
	"time"

	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

const (
	cacheStateProgName  = "cachestat"
	runQLatencyProgName = "runqlatency"
	cpuSchedProgName    = "cpusched"
	processVMMProgName  = "processvm"
	oomKillProgName     = "oomkill"
	bioProgName         = "bio"
	profileProgName     = "profile"
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

var (
	__powerOfTwo = [20]int{1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535, 131071, 262143, 524287, 1048575}
)
