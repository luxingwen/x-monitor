/*
 * @Author: CALM.WU
 * @Date: 2023-02-17 14:33:02
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 16:13:11
 */

package collector

import (
	"github.com/cilium/ebpf/link"
	"github.com/golang/glog"
	"github.com/sourcegraph/conc"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

type eBPFBaseProgram struct {
	// module
	name        string
	stopChan    chan struct{}
	wg          conc.WaitGroup
	gatherTimer *calmutils.Timer
	// ebpf
	links []link.Link
}

func (ebm *eBPFBaseProgram) finalizer() {
	close(ebm.stopChan)
	if recover := ebm.wg.WaitAndRecover(); recover != nil {
		glog.Errorf("eBPFProgram:'%s' recover: %v", ebm.name, recover.String())
	}

	if ebm.links != nil {
		for _, link := range ebm.links {
			link.Close()
		}
		ebm.links = nil
	}

	if ebm.gatherTimer != nil {
		ebm.gatherTimer.Stop()
		ebm.gatherTimer = nil
	}
}
