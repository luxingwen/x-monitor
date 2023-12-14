/*
 * @Author: CALM.WU
 * @Date: 2023-09-06 15:19:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-12-13 11:22:59
 */

package pyrostub

import (
	"testing"
)

// go test  -timeout 30s -run ^TestNewPyroscopeExporter$ xmonitor.calmwu/plugin_ebpf/exporter/internal/pyrostub -v -logtostderr
func TestNewPyroscopeExporter(t *testing.T) {
	url := "127.0.0.1:4040"
	err := InitPyroscopeExporter(url)
	if err != nil {
		t.Fatal("NewPyroscopeExporter failed.")
	}
	t.Log("NewPyroscopeExporter successed.")
}
