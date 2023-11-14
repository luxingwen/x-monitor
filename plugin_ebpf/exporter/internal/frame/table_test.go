/*
 * @Author: CALM.WU
 * @Date: 2023-11-03 10:52:23
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-11-03 10:59:07
 */

package frame

import (
	"testing"

	"github.com/parca-dev/parca-agent/pkg/stack/unwind"
	"golang.org/x/exp/slices"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
)

// go test -v -run=TestSortFdeInfos -v -logtostderr
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

// go test -v -run=TestDumpModuleFDETables -v -logtostderr
func TestDumpModuleFDETables(t *testing.T) {
	module := "/usr/lib64/libc-2.28.so"

	fdeTables, err := CreateModuleFDETables(module)
	if err != nil {
		t.Fatal(err)
	}

	t.Logf("module:'%s' has %d fde tables", module, fdeTables.FdeTableCount)
	var i uint32 = 0
	for ; i < fdeTables.FdeTableCount; i++ {
		info := &fdeTables.FdeInfos[i]
		t.Logf("table{start:'%#x---end:%#x'} row count:%d, row pos:%d", info.Start, info.End, info.RowCount, info.RowPos)
		if info.RowCount == 1 {
			t.Logf("\tRow:%#v", fdeTables.FdeRows[info.RowPos])
		}
	}
}

// go test -v -run=TestDumpModuleFDETableRowsWithParca -v -logtostderr
func TestDumpModuleFDETableRowsWithParca(t *testing.T) {
	module := "/usr/lib64/libc-2.28.so"

	rows, _, err := unwind.GenerateCompactUnwindTable(module)
	if err != nil {
		t.Fatal(err)
	}

	for _, row := range rows {
		t.Logf("Row:'%s'", row.ToString(true))
	}
}
