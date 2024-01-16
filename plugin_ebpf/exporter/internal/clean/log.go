/*
 * @Author: CALM.WU
 * @Date: 2024-01-15 15:23:22
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-01-15 18:25:27
 */

package clean

import (
	"context"
	"os"
	"path/filepath"
	"sort"
	"strings"
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
		Period:      time.Hour,
		FilterTags:  []string{"x-monitor.ebpf"},
		LevelReservedCount: LevelReservedCount{
			Info: 3,
			Err:  2,
			Warn: 2,
		},
	}
}

func Start(ctx context.Context, opts *Options) {
	// 判断 LogFullPath 是否是一个链接目录
	if info, err := os.Lstat(opts.LogFullPath); err == nil {
		if info.Mode()&os.ModeSymlink != 0 {
			// 读取实际的目录
			if opts.LogFullPath, err = os.Readlink(opts.LogFullPath); err != nil {
				glog.Error(err.Error())
				return
			} else {
				glog.Infof("logDir link ===> %s", opts.LogFullPath)
			}
		}
		ctx = context.WithValue(ctx, archiveContextKey("opts"), opts)
		go calmutils.NonSlidingUntilWithContext(ctx, __clean, opts.Period)
	} else {
		glog.Error(err.Error())
	}

}

func __clean(ctx context.Context) {
	opts, ok := ctx.Value(archiveContextKey("opts")).(*Options)
	if !ok || opts == nil {
		glog.Info("opts is nil")
		return
	}

	glog.Infof("opts:%v", opts)

	var logFiles []fileWithTime

	// 扫描目录，列出文件
	err := filepath.Walk(opts.LogFullPath, func(path string, info os.FileInfo, err error) error {
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

	// 从新到旧排序
	sort.Slice(logFiles, func(i, j int) bool {
		return logFiles[i].Time.After(logFiles[j].Time)
	})

	errReservedCount := opts.Err
	warnReservedCount := opts.Warn
	infoReservedCount := opts.Info

	pfnRemove := func(path string) {
		if err := os.Remove(path); err != nil {
			glog.Errorf("remove file failed. err:%s", err.Error())
		} else {
			glog.Infof("remove file:%s successed.", path)
		}
	}

	for _, l := range logFiles {
		for _, ft := range opts.FilterTags {
			if strings.Contains(l.Path, ft) {
				if strings.Contains(l.Path, ".INFO.") {
					if infoReservedCount == 0 {
						pfnRemove(l.Path)
						continue
					}
					infoReservedCount--
				} else if strings.Contains(l.Path, ".WARNING.") {
					if warnReservedCount == 0 {
						pfnRemove(l.Path)
						continue
					}
					warnReservedCount--
				} else if strings.Contains(l.Path, ".ERROR.") {
					if errReservedCount == 0 {
						pfnRemove(l.Path)
						continue
					}
					errReservedCount--
				}
			}
		}
	}
}
