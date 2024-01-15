/*
 * @Author: CALM.WU
 * @Date: 2024-01-15 15:23:22
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-01-15 18:25:27
 */

package log

import (
	"context"
	"os"
	"path/filepath"
	"sort"
	"time"

	"github.com/golang/glog"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

type LevelReservedCount struct {
	Info int
	Err  int
	Warn int
}

type Options struct {
	LevelReservedCount               // 保留数量
	LogFullPath        string        // 日志文件全路径
	Period             time.Duration // 执行周期
	FilterTags         []string      // 日志文件名匹配
}

type fileWithTime struct {
	Path string
	Time time.Time
}

// fix SA1029
type archiveContextKey string

func DefaultOptions() *Options {
	return &Options{
		LogFullPath: "/var/log/x-monitor",
		Period:      24 * time.Hour,
		FilterTags:  []string{"x-monitor.ebpf"},
		LevelReservedCount: LevelReservedCount{
			Info: 3,
			Err:  2,
			Warn: 2,
		},
	}
}

func Start(ctx context.Context, options *Options) {
	ctx = context.WithValue(ctx, archiveContextKey("opts"), options)
	go calmutils.NonSlidingUntilWithContext(ctx, __archiving, options.Period)
}

func __archiving(ctx context.Context) {
	options, ok := ctx.Value(archiveContextKey("opts")).(*Options)
	if !ok || options == nil {
		glog.Info("opts is nil")
		return
	}

	glog.Infof("opts:%v", options)

	var logFiles []fileWithTime

	// 扫描目录，列出文件
	err := filepath.Walk(options.LogFullPath, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		// 过滤掉目录
		if info.IsDir() {
			return nil
		}

		// 过滤掉链接文件
		if info.Mode()&os.ModeSymlink != 0 {
			return nil
		}

		logFiles = append(logFiles, fileWithTime{Path: path, Time: info.ModTime()})
		return nil
	})

	if err != nil {
		glog.Error(err.Error())
	}

	glog.Infof("logFiles count:%d", len(logFiles))

	// 按时间排序
	sort.Slice(logFiles, func(i, j int) bool {
		return logFiles[i].Time.Before(logFiles[j].Time)
	})

	for _, l := range logFiles {
		glog.Infof("%s, last mod time:%s", l.Path, l.Time.String())
	}
}
