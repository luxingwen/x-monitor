/*
 * @Author: CALM.WU
 * @Date: 2024-01-15 17:00:37
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-01-15 18:24:04
 */

package clean

import (
	"context"
	"testing"
	"time"

	"github.com/golang/glog"
)

// go test -run ^TestCleanLogs$ xmonitor.calmwu/plugin_ebpf/exporter/internal/clean -v -logtostderr
func TestCleanLogs(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute*5)
	defer cancel()

	options := DefaultOptions()
	options.LogFullPath = "/var/log/x-monitor"
	options.Period = time.Minute

	glog.Info("TestCleanLogs start")

	Start(ctx, options)

	<-ctx.Done()

	glog.Info("TestCleanLogs done")
}
