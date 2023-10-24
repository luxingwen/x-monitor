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

// go test -v -timeout 30s -run ^TestString2Int8Array$ xmonitor.calmwu/plugin_ebpf/exporter/internal/utils
func TestString2Int8Array(t *testing.T) {
	s := "12345678"
	array := [8]int8{}

	t.Logf("array to slice len:%d", len(array[:]))

	String2Int8Array(s, array[:])
	// [8]int8{49, 50, 51, 52, 53, 54, 55, 0}
	t.Logf("array: %#v", array)

	s = "1234"
	String2Int8Array(s, array[:])
	t.Logf("array: %#v", array)

	s = "1234567890"
	String2Int8Array(s, array[:])
	t.Logf("array: %#v", array)
}
