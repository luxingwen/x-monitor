/*
 * @Author: CALM.WU
 * @Date: 2023-09-06 11:14:32
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-06 14:34:44
 */

package pyrostub

import (
	"testing"

	"github.com/go-kit/log"
	"github.com/go-kit/log/level"
)

// go test  -timeout 30s -run ^TestLoggerAdapter$ xmonitor.calmwu/plugin_ebpf/exporter/internal/pyrostub -v -logtostderr
func TestLoggerAdapter(t *testing.T) {
	logger := log.With(__defaultLogger, "author", "calmwu")
	level.Debug(logger).Log("msg", "test logger adapter")
}
