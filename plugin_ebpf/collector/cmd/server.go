/*
 * @Author: CALM.WU
 * @Date: 2023-02-06 11:39:12
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-06 16:53:30
 */

package cmd

import (
	goflag "flag"
	"fmt"
	"os"

	"github.com/golang/glog"
	"github.com/spf13/cobra"
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

func Main() {
	if err := __rootCmd.Execute(); err != nil {
		glog.Fatal(err.Error())
	}
}

func __rootCmdRun(cmd *cobra.Command, args []string) {
	defer glog.Flush()

	glog.Info("Hi~~~, xm-monitor eBPF metrics collector.")
	// 解析配置文件
}
