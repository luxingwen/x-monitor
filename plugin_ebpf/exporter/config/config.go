/*
 * @Author: CALM.WU
 * @Date: 2023-02-08 11:41:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-10 16:25:05
 */

package config

import (
	"fmt"
	"sync"

	"github.com/fsnotify/fsnotify"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/sanity-io/litter"
	"github.com/spf13/viper"
	"github.com/vishvananda/netlink"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

type viperDebugAdapterLog struct{}

func (v *viperDebugAdapterLog) Write(p []byte) (n int, err error) {
	glog.Info(calmutils.Bytes2String(p))
	return len(p), nil
}

type EBPFModuleConfig struct {
	Name    string `mapstructure:"name"`
	Enabled bool   `mapstructure:"enabled"`
}

var (
	__ebpfModuleConfigs []*EBPFModuleConfig
	__mu                sync.RWMutex
)

func InitConfig(cfgFile string) error {
	viper.SetConfigFile(cfgFile)
	if err := viper.ReadInConfig(); err != nil {
		err = errors.Wrapf(err, "read config file %s", cfgFile)
		glog.Error(err)
		return err
	}

	viper.DebugTo(&viperDebugAdapterLog{})

	if err := viper.UnmarshalKey("ebpf.modules", &__ebpfModuleConfigs); err != nil {
		err = errors.Wrapf(err, "unmarshal key ebpf.modules")
		glog.Error(err)
		return err
	}

	// 监控配置文件变化
	viper.WatchConfig()
	viper.OnConfigChange(func(e fsnotify.Event) {
		glog.Infof("Config file changed: %s", e.Name)

		// update
		__mu.Lock()
		defer __mu.Unlock()
		var tmpCfgs []*EBPFModuleConfig
		if err := viper.UnmarshalKey("ebpf.modules", &tmpCfgs); err != nil {
			err = errors.Wrapf(err, "unmarshal key ebpf.modules")
			glog.Error(err)
		} else {
			__ebpfModuleConfigs = nil
			__ebpfModuleConfigs = tmpCfgs
			glog.Infof("ebpf.modules %s", litter.Sdump(__ebpfModuleConfigs))
		}
	})

	return nil
}

func __getIP(assignType string) (string, error) {
	switch assignType {
	case "ip":
		return viper.GetString("net.ip.value"), nil
	case "itf_name":
		return calmutils.GetIPByIfname(viper.GetString("net.ip.value"))
	case "default_route":
		routeList, err := netlink.RouteList(nil, netlink.FAMILY_V4)
		if err != nil {
			return "", errors.Wrap(err, "netlink.RouteList netlink.FAMILY_V4")
		} else {
			for _, route := range routeList {
				if route.Dst == nil {
					// Dst == nil是默认路由
					link, err := netlink.LinkByIndex(route.LinkIndex)
					if err == nil {
						addrs, err := netlink.AddrList(link, netlink.FAMILY_V4)
						if err == nil {
							// 获取第一个地址
							return addrs[0].IP.String(), nil
						} else {
							return "", errors.Wrap(err, "netlink.AddrList")
						}
					} else {
						return "", errors.Wrap(err, "netlink.LinkByIndex")
					}
				}
			}
			return "", errors.New("Not found default route")
		}
	}
	return "", errors.Errorf("Not support get ip by assign type: '%s'", assignType)
}

// PProfBindAddr returns the IP address and port that the pprof endpoint is configured to listen on.
// The IP address is determined by the net.ip.assignType configuration value.
func PProfBindAddr() (string, error) {
	assignType := viper.GetString("net.ip.assignType")
	ip, err := __getIP(assignType)
	if err != nil {
		return "", err
	}
	port := viper.GetInt("net.port.pprof")
	return fmt.Sprintf("%s:%d", ip, port), nil
}

// APISrvBindAddr returns the address to bind the API server to. It uses the
// user-specified bind address, if one was provided, or it uses an IP address
// assigned by the user-specified IP assignment method.
func APISrvBindAddr() (string, error) {
	assignType := viper.GetString("net.ip.assignType")
	ip, err := __getIP(assignType)
	if err != nil {
		return "", err
	}
	port := viper.GetInt("net.port.api")
	return fmt.Sprintf("%s:%d", ip, port), nil
}

// PromMetricsPath returns the path to the prometheus metrics endpoint
func PromMetricsPath() string {
	return viper.GetString("api.path.metric")
}

// Returns true if the eBPF exporter is enabled.
func ExporterEnabled() bool {
	return viper.GetBool("ebpf.enabled")
}

// EBPFModuleConfigs unmarshals the ebpf.modules section of the configuration file
// into a slice of EBPFModuleConfig. If the section is not present in the config,
// it returns an empty slice.
func EBPFModuleEnabled(name string) bool {
	__mu.RLock()
	defer __mu.Unlock()

	for _, moduleCfg := range __ebpfModuleConfigs {
		if moduleCfg.Name == name {
			return moduleCfg.Enabled
		}
	}
	return false
}
