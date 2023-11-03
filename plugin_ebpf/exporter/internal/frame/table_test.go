/*
 * @Author: CALM.WU
 * @Date: 2023-11-03 10:52:23
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-11-03 10:59:07
 */

package frame

import (
	"testing"

	"golang.org/x/exp/slices"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

// GO111MODULE=off go test -v -run=TestSortFdeInfos -v -logtostderr
func TestSortFdeInfos(t *testing.T) {
	FdeInfos := [6]bpfmodule.XMProfileXmProfileFdeTableInfo{
		{Start: 10, RowCount: 19},
		{Start: 2, RowCount: 2},
		{Start: 200, RowCount: 200},
		{Start: 200, RowCount: 200},
		{Start: 0, RowCount: 0},
		{Start: 0, RowCount: 0},
	}
	count := 4

	slices.SortFunc(FdeInfos[0:count], func(a, b bpfmodule.XMProfileXmProfileFdeTableInfo) bool { return a.Start < b.Start })

	for i, fi := range FdeInfos {
		t.Logf("i:%d fi:%#v", i, fi)
	}
}
