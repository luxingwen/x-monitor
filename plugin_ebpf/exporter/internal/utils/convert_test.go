/*
 * @Author: CALM.WU
 * @Date: 2023-08-18 15:23:45
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 15:30:42
 */

package utils

import (
	"testing"
)

// go test -v -timeout 30s -run ^TestCommToString$ xmonitor.calmwu/plugin_ebpf/exporter/internal/utils
func TestCommToString(t *testing.T) {
	var commSlice []int8 = []int8{115, 121, 115, 116, 101, 109, 100, 0, 0, 0, 0, 0, 0, 0, 0}
	t.Log(CommToString(commSlice))
}
