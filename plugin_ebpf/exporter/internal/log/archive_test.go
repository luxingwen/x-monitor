/*
 * @Author: CALM.WU
 * @Date: 2024-01-15 17:00:37
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-01-15 18:24:04
 */

package log

import (
	"context"
	"testing"
	"time"

	"github.com/golang/glog"
)

// go test  -timeout 30s -run ^TestArchiveLogs$ xmonitor.calmwu/plugin_ebpf/exporter/internal/log -v -logtostderr
func TestArchiveLogs(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute*5)
	defer cancel()

	options := DefaultOptions()
	options.LogFullPath = "/home/pingan/Program/x-monitor/log"
	options.Period = time.Minute

	glog.Info("TestArchiveLogs start")

	Start(ctx, options)

	<-ctx.Done()

	glog.Info("TestArchiveLogs done")
}
