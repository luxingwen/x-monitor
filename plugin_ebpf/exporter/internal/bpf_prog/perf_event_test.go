/*
 * @Author: CALM.WU
 * @Date: 2023-08-18 17:24:20
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 17:24:48
 */

package bpfprog

import (
	"testing"
	"time"

	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

const (
	mapKey uint32 = 0
)

// go test  -timeout 30s -run ^TestPerfEventProg$ xmonitor.calmwu/plugin_ebpf/exporter/internal/bpf_prog -v -logtostderr
func TestPerfEventProg(t *testing.T) {
	objs := bpfmodule.XMProfileObjects{}

	if err := bpfmodule.LoadXMProfileObjects(&objs, nil); err != nil {
		t.Fatalf("load objects: %v", err)
	}

	t.Log("load XMProfile objects successed")

	defer objs.Close()

	if links, err := AttachPerfEventProg(-1, 99, objs.XmDoPerfEvent); err != nil {
		t.Fatalf("attach PerfEvent Prog: %s", err)
	} else {

		for i := int(bpfmodule.XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_OS); i < int(bpfmodule.XMProfileXmProgFilterTargetScopeTypeXM_PROG_FILTER_TARGET_SCOPE_TYPE_MAX); i++ {

			args := new(bpfmodule.XMProfileXmProgFilterArg)
			args.ScopeType = bpfmodule.XMProfileXmProgFilterTargetScopeType(i)
			args.FilterCondValue = uint64(i)

			if err := objs.XmProfileArgMap.Update(mapKey, args, 0); err != nil {
				t.Fatalf("update profile args map failed. err:%s", err)
			} else {
				t.Logf("udpate profiel args map.ScopeType: %d successed", args.ScopeType)
			}

			time.Sleep(1 * time.Second)
		}

		links.Detach()
	}
}
