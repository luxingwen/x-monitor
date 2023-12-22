/*
 * @Author: CALM.WU
 * @Date: 2023-12-21 14:45:03
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-12-21 15:20:00
 */

package main

import (
	"debug/elf"
	goflag "flag"
	"fmt"

	dlvFrame "github.com/go-delve/delve/pkg/dwarf/frame"
	"github.com/golang/glog"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

var (
	__pid int
)

func init() {
	goflag.IntVar(&__pid, "pid", -1, "pid of process")
}

func pointerSize(arch elf.Machine) int {
	//nolint:exhaustive
	switch arch {
	case elf.EM_386:
		return 4
	case elf.EM_AARCH64, elf.EM_X86_64:
		return 8
	default:
		return 0
	}
}

func main() {
	goflag.Parse()

	glog.Infof("Start counting '%d' the number of modules and the number of fde tables in each module", __pid)

	calmutils.InitModuleSymbolTblMgr(128)

	if procSyms, err := calmutils.NewProcSyms(__pid); err == nil {
		modules := procSyms.Modules()

		glog.Infof("pid:%d have %d modules", __pid, len(modules))

		statisticsMap := make(map[string]int, len(modules))

		for _, m := range modules {
			modulePath := fmt.Sprintf("%s%s", m.RootFS, m.Pathname)
			statisticsMap[modulePath] = 0

			if ef, err := elf.Open(modulePath); err == nil {
				section := ef.Section(".eh_frame")
				if data, err := section.Data(); err == nil {
					if fdes, err := dlvFrame.Parse(data, ef.ByteOrder, 0, pointerSize(ef.Machine), section.Addr); err == nil {
						statisticsMap[modulePath] = len(fdes)
					}
				}
				ef.Close()
			}
		}

		for k, v := range statisticsMap {
			glog.Infof("module:%s fde table count:%d", k, v)
		}
	} else {
		glog.Errorf("open /proc/%d/maps. err:%s", __pid, err.Error())
	}
}
