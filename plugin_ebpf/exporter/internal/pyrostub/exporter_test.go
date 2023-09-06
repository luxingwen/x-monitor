/*
 * @Author: CALM.WU
 * @Date: 2023-09-06 15:19:47
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2023-09-06 15:19:47
 */

package pyrostub

import (
	"testing"

	"github.com/prometheus/client_golang/prometheus"
)

// go test  -timeout 30s -run ^TestNewPyroscopeExporter$ xmonitor.calmwu/plugin_ebpf/exporter/internal/pyrostub -v -logtostderr
func TestNewPyroscopeExporter(t *testing.T) {
	url := "127.0.0.1:4040"
	_, err := NewPyroscopeExporter(url, prometheus.NewRegistry())
	if err != nil {
		t.Fatal("NewPyroscopeExporter failed.")
	}
	t.Log("NewPyroscopeExporter successed.")
}
