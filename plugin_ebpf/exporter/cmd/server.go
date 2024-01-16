/*
 * @Author: CALM.WU
 * @Date: 2023-02-06 11:39:12
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-01-15 16:08:35
 */

package cmd

import (
	"bytes"
	goflag "flag"
	"fmt"
	"net/http"
	"os"

	"github.com/cilium/ebpf/rlimit"
	"github.com/fatih/color"
	"github.com/gin-gonic/gin"
	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
	"github.com/prometheus/common/version"
	"github.com/spf13/cobra"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"golang.org/x/sync/singleflight"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/clean"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/eventcenter"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/frame"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/net"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/pyrostub"
)

var (
	// Version Major string
	_VersionMajor string
	// _VersionMinor string version
	_VersionMinor string
	// Branch name.
	_BranchName string
	// _CommitHash hash string
	_CommitHash string
	// _BuildTime string.
	_BuildTime string
	// 配置文件
	_configFile string
	// pyroscope 服务器地址
	_pyroscope string
	// Defining the root command for the CLI.
	_rootCmd = &cobra.Command{
		Use:  "x-monitor.eBPF application",
		Long: "x-monitor plugin application for eBPF metrics collector",
		Version: func() string {
			return fmt.Sprintf("\n\tVersion: %s.%s\n\tGit: %s:%s\n\tBuild Time: %s\n",
				_VersionMajor, _VersionMinor, _BranchName, _CommitHash, _BuildTime)
		}(),
		Run: rootCmdRun,
	}

	_APIHttp       *net.WebSrv
	_EBPFCollector *collector.EBPFCollector
	_gf            = singleflight.Group{}
)

func init() {
	_rootCmd.Flags().StringVar(&_configFile, "config", "", "config.yaml")
	_rootCmd.Flags().StringVar(&_pyroscope, "pyroscope", "", "pyroscope server address")
	_rootCmd.Flags().AddGoFlagSet(goflag.CommandLine)
	_rootCmd.Flags().Parse(os.Args[1:])
}

// Main is the entry point for the application.
func Main() {
	if err := _rootCmd.Execute(); err != nil {
		glog.Fatal("%s %v\n\n", color.RedString("Error:"), err)
	}
}

// registerPromCollectors registers the collectors used by the Prometheus
// metrics endpoint. It sets the metrics version to the application version,
// and also registers the standard Go and Process collectors.
func registerPromCollectors() {
	version.Version = fmt.Sprintf("%s.%s", _VersionMajor, _VersionMinor)
	version.Revision = _CommitHash
	version.Branch = _BranchName
	version.BuildDate = _BuildTime

	// 注册 collectors
	prometheus.MustRegister(version.NewCollector("xmonitor_eBPF"))

	var err error

	_EBPFCollector, err = collector.New()
	if err != nil {
		glog.Fatalf("Couldn't create eBPF collector: %s", err.Error())
	}

	if err = prometheus.Register(_EBPFCollector); err != nil {
		glog.Fatalf("Couldn't register eBPF collector: %s", err.Error())
	}
}

func prometheusHandler() gin.HandlerFunc {
	h := promhttp.Handler()

	return func(c *gin.Context) {
		h.ServeHTTP(c.Writer, c.Request)
	}
}

// rootCmdRun is the main entry of xm-monitor.eBPF collector.
// It is responsible for initializing the configuration file, loading the kernel symbols, and setting up the signal handler.
// It also installs the pprof module for debugging.
func rootCmdRun(cmd *cobra.Command, args []string) {
	defer glog.Flush()

	glog.Info("Hi~~~, xm-monitor.eBPF metrics collector.")

	// Config
	if err := config.InitConfig(_configFile); err != nil {
		glog.Fatal(err.Error())
	}

	config.PyroscopeSrvAddr(_pyroscope)
	if err := pyrostub.InitPyroscopeExporter(config.PyroscopeSvrAddr()); err != nil {
		glog.Fatalf("Init Pytoscope export failed, reason:%s", err.Error())
	}

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err.Error())
	}

	// init symbols
	if err := frame.InitSystemSymbolsCache(32); err != nil {
		glog.Fatalf("Init Symbols Cache failed. %s", err.Error())
	}

	// Signal
	ctx := calmutils.SetupSignalHandler()

	// log clean
	opts := clean.DefaultOptions()
	flag := goflag.CommandLine.Lookup("log_dir")
	if flag != nil {
		val := flag.Value.String()
		if val != "" {
			opts.LogFullPath = val
		}
	}
	//opts.LogFullPath =
	clean.Start(ctx, opts)

	// Install pprof
	bind, _ := config.PProfBindAddr()
	calmutils.InstallPProf(bind)

	//
	eventcenter.Init()

	// 注册 prometheus collectors
	registerPromCollectors()

	// 启动 web 服务
	bind, _ = config.APISrvBindAddr()
	_APIHttp = net.NewWebSrv("x-monitor.eBPF", ctx, bind, false, "", "")

	// 注册 router
	metricsPath := config.PromMetricsPath()
	_APIHttp.Handle(http.MethodGet, metricsPath, prometheusHandler())
	_APIHttp.Handle(http.MethodGet, "/", func(c *gin.Context) {
		name := c.Request.URL.Query().Get("name")
		response, _, _ := _gf.Do(name, func() (interface{}, error) {
			b := bytes.NewBuffer([]byte(`<html>
			<head><title>x-monitor.eBPF</title></head>
			<body>
			<h1>x-monitor.eBPF Exporter</h1>
			<p><a href="` + metricsPath + `">Metrics</a></p>
			</body>
			</html>`))
			return b, nil
		})

		_, err := response.(*bytes.Buffer).WriteTo(c.Writer)
		if err != nil {
			glog.Fatalf("write response failed, err: %s", err.Error())
		}
	})

	_APIHttp.Start()

	<-ctx.Done()

	_APIHttp.Stop()

	_EBPFCollector.Stop()

	eventcenter.Stop()

	glog.Info("xm-monitor.eBPF exporter exit!")
}
