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
	"runtime/debug"

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
	VersionMajor string
	// VersionMinor string version
	VersionMinor string
	// Branch name.
	BranchName string
	// CommitHash hash string
	CommitHash string
	// BuildTime string.
	BuildTime string
	// 配置文件
	configFile string
	// pyroscope 服务器地址
	pyroscope string
	// Defining the root command for the CLI.
	rootCmd = &cobra.Command{
		Use:  "x-monitor.eBPF application",
		Long: "x-monitor plugin application for eBPF metrics collector",
		Version: func() string {
			return fmt.Sprintf("\n\tVersion: %s.%s\n\tGit: %s:%s\n\tBuild Time: %s\n",
				VersionMajor, VersionMinor, BranchName, CommitHash, BuildTime)
		}(),
		Run: rootCmdRun,
	}

	apiHttp       *net.WebSrv
	eBPFCollector *collector.EBPFCollector
	gf            = singleflight.Group{}
)

func init() {
	rootCmd.Flags().StringVar(&configFile, "config", "", "config.yaml")
	rootCmd.Flags().StringVar(&pyroscope, "pyroscope", "", "pyroscope server address")
	rootCmd.Flags().AddGoFlagSet(goflag.CommandLine)
	rootCmd.Flags().Parse(os.Args[1:])
}

// Main is the entry point for the application.
func Main() {
	if err := rootCmd.Execute(); err != nil {
		glog.Fatal("%s %v\n\n", color.RedString("Error:"), err)
	}
}

// registerPromCollectors registers the collectors used by the Prometheus
// metrics endpoint. It sets the metrics version to the application version,
// and also registers the standard Go and Process collectors.
func registerPromCollectors() {
	version.Version = fmt.Sprintf("%s.%s", VersionMajor, VersionMinor)
	version.Revision = CommitHash
	version.Branch = BranchName
	version.BuildDate = BuildTime

	// 注册 collectors
	prometheus.MustRegister(version.NewCollector("xmonitor_eBPF"))

	var err error

	eBPFCollector, err = collector.New()
	if err != nil {
		glog.Fatalf("Couldn't create eBPF collector: %s", err.Error())
	}

	if err = prometheus.Register(eBPFCollector); err != nil {
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
	if err := config.InitConfig(configFile); err != nil {
		glog.Fatal(err.Error())
	}

	memoryLimit := config.MemoryLimit()
	priorMemoryLimit := debug.SetMemoryLimit(memoryLimit)
	glog.Infof("Set memory limit to %d bytes, prior memory limit %d bytes", memoryLimit, priorMemoryLimit)

	config.SetPyroscopeSrvAddr(pyroscope)
	if err := pyrostub.InitPyroscopeExporter(config.PyroscopeSrvAddr()); err != nil {
		glog.Fatalf("Init Pytoscope export failed, reason:%s", err.Error())
	}

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err.Error())
	}

	// init symbols
	if err := frame.InitSystemSymbolsCache(); err != nil {
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
	apiHttp = net.NewWebSrv("x-monitor.eBPF", ctx, bind, false, "", "")

	// 注册 router
	metricsPath := config.PromMetricsPath()
	apiHttp.Handle(http.MethodGet, metricsPath, prometheusHandler())
	apiHttp.Handle(http.MethodGet, "/", func(c *gin.Context) {
		name := c.Request.URL.Query().Get("name")
		response, _, _ := gf.Do(name, func() (interface{}, error) {
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

	apiHttp.Start()

	<-ctx.Done()

	apiHttp.Stop()

	eBPFCollector.Stop()

	eventcenter.Stop()

	glog.Info("xm-monitor.eBPF exporter exit!")
}
