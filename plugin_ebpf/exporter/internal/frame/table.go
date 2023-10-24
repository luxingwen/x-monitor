/*
 * @Author: CALM.WU
 * @Date: 2023-10-24 14:47:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-10-24 15:07:18
 */

package frame

import (
	"github.com/golang/glog"
	"github.com/pkg/errors"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/utils"
)

const (
	__maxPerModuleFDETableCount     = 2048 // 每个module包含的fde table最大数量
	__maxPerProcessAssocModuleCount = 32   // 每个进程关联的module最大数量
)

func GetProcessMaps(pid int32) (*bpfmodule.XMProfileXmPidMaps, error) {
	procMaps := new(bpfmodule.XMProfileXmPidMaps)

	if modules, err := __getProcModules(pid); err != nil {
		return nil, errors.Wrap(err, "get process modules failed.")
	} else {

		for i, module := range modules {
			procMaps.Modules[i].StartAddr = module.StartAddr
			procMaps.Modules[i].EndAddr = module.EndAddr
			procMaps.Modules[i].BuildIdHash = calmutils.HashStr2Uint64(module.BuildID)
			utils.String2Int8Array(module.Pathname, procMaps.Modules[i].Path[:])

			procMaps.ModuleCount += 1
			if procMaps.ModuleCount >= __maxPerProcessAssocModuleCount {
				glog.Warning("too many modules associated with process, ignore the rest.")
				break
			}
		}
	}

	return procMaps, nil
}

func CreateFDETablesForModule(modulePath string) (*bpfmodule.XMProfileXmProfileModuleFdeTables, error) {
	return nil, nil
}
