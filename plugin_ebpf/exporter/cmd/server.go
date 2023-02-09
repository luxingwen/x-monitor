/*
 * @Author: CALM.WU
 * @Date: 2023-02-06 11:39:12
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-09 16:59:25
 */

package cmd

import (
	goflag "flag"
	"fmt"
	"os"

	"github.com/cilium/ebpf/rlimit"
	"github.com/golang/glog"
	"github.com/prometheus/client_golang/prometheus"
	promcollectors "github.com/prometheus/client_golang/prometheus/collectors"
	"github.com/prometheus/common/version"
	"github.com/spf13/cobra"

	"xmonitor.calmwu/plugin_ebpf/exporter/collector"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"

	calmutils "github.com/wubo0067/calmwu-go/utils"
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

	__configFile string

	// Defining the root command for the CLI.
	__rootCmd = &cobra.Command{
		Use:  "xm-monitor.eBPF application",
		Long: "xm-monitor plugin application for eBPF metrics collector",
		Version: func() string {
			return fmt.Sprintf("\n\tVersion: %s.%s\n\tGit: %s:%s\n\tBuild Time: %s\n",
				VersionMajor, VersionMinor, BranchName, CommitHash, BuildTime)
		}(),
		Run: __rootCmdRun,
	}
)

func init() {
	__rootCmd.Flags().StringVar(&__configFile, "config", "", "env/config/xm_ebpf_plugin/config.json")
	__rootCmd.Flags().AddGoFlagSet(goflag.CommandLine)
	__rootCmd.Flags().Parse(os.Args[1:])
}

// Main is the entry point for the application.
func Main() {
	if err := __rootCmd.Execute(); err != nil {
		glog.Fatal(err.Error())
	}
}

// __registerPromCollectors registers the collectors used by the Prometheus
// metrics endpoint. It sets the metrics version to the application version,
// and also registers the standard Go and Process collectors.
func __registerPromCollectors() {
	version.Version = fmt.Sprintf("%s.%s", VersionMajor, VersionMinor)
	version.Revision = CommitHash
	version.Branch = BranchName
	version.BuildDate = BuildTime

	prometheus.MustRegister(version.NewCollector("xm-monitor.eBPF"),
		promcollectors.NewProcessCollector(promcollectors.ProcessCollectorOpts{}),
		promcollectors.NewGoCollector(),
	)

	epbfCollector, _ := collector.NewEbpfCollector()
	if err := prometheus.Register(epbfCollector); err != nil {
		glog.Fatalf("Couldn't register eBPF collector: %s", err.Error())
	}
}

// __rootCmdRun is the main entry of xm-monitor.eBPF collector.
// It is responsible for initializing the configuration file, loading the kernel symbols, and setting up the signal handler.
// It also installs the pprof module for debugging.
func __rootCmdRun(cmd *cobra.Command, args []string) {
	defer glog.Flush()

	glog.Info("Hi~~~, xm-monitor.eBPF metrics collector.")

	// Config
	if err := config.InitConfig(__configFile); err != nil {
		glog.Fatal(err.Error())
	}

	if err := rlimit.RemoveMemlock(); err != nil {
		glog.Fatalf("failed to remove memlock limit: %v", err)
	}

	// Load kernel symbols
	if err := calmutils.LoadKallSyms(); err != nil {
		glog.Fatal(err.Error())
	} else {
		glog.Info("Load kernel symbols success!")
	}

	// Signal
	ctx := calmutils.SetupSignalHandler()

	// Install pprof
	calmutils.InstallPProf(config.GetPProfBindAddr())

	//
	__registerPromCollectors()

	<-ctx.Done()

	glog.Info("xm-monitor.eBPF collector exit!")
}
