/*
 * @Author: CALM.WU
 * @Date: 2023-02-08 11:41:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-27 11:28:13
 */

package config

import (
	"fmt"
	"sync"
	"time"

	"github.com/fsnotify/fsnotify"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/sanity-io/litter"
	"github.com/spf13/viper"
	"github.com/vishvananda/netlink"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

type viperDebugAdapterLog struct{}

const (
	defaultInterval = 10 * time.Second
)

type XMInternalResourceType int

//go:generate stringer -type=XMInternalResourceType -output=xm_internalresourcetype_string.go
const (
	XM_Resource_None XMInternalResourceType = iota
	XM_Resource_OS
	XM_Resource_Namespace
	XM_Resource_Cgroup
	XM_Resource_PID
	XM_Resource_PGID //进程组
)

func (v *viperDebugAdapterLog) Write(p []byte) (n int, err error) {
	glog.Info(calmutils.Bytes2String(p))
	return len(p), nil
}

type ProgRodataBase struct {
	FilterScopeType  XMInternalResourceType `mapstructure:"filter_scope_type"`  // 过滤的资源类型
	FilterScopeValue int                    `mapstructure:"filter_scope_value"` // 过滤资源的值
}

type ProgConfigFilter struct {
	ProgRodata   interface{} `mapstructure:"prog_rodata"`
	IncludeComms []string    `mapstructure:"include_comms"`
	ExcludeComms []string    `mapstructure:"exclude_comms"`
	ObjectCount  int         `mapstructure:"object_count"`
}

type ProgramConfig struct {
	Name           string           `mapstructure:"name"`
	Metrics        []string         `mapstructure:"metrcs"`
	Filter         ProgConfigFilter `mapstructure:"filter"`
	GatherInterval time.Duration    `mapstructure:"gather_interval"`
	Enabled        bool             `mapstructure:"enabled"`
}

var (
	programConfigs []*ProgramConfig
	mu             sync.RWMutex
)

func InitConfig(cfgFile string) error {
	viper.SetConfigFile(cfgFile)
	if err := viper.ReadInConfig(); err != nil {
		err = errors.Wrapf(err, "read config file %s", cfgFile)
		glog.Error(err)
		return err
	}

	viper.DebugTo(&viperDebugAdapterLog{})

	if err := viper.UnmarshalKey("ebpf.programs", &programConfigs); err != nil {
		err = errors.Wrapf(err, "unmarshal key ebpf.programs")
		glog.Error(err)
		return err
	} else {
		glog.Infof("ebpf.programs %s", litter.Sdump(programConfigs))
	}

	// 监控配置文件变化
	viper.WatchConfig()
	viper.OnConfigChange(func(e fsnotify.Event) {
		glog.Infof("Config file changed: %s", e.Name)

		// update
		mu.Lock()
		defer mu.Unlock()
		var tmpCfgs []*ProgramConfig
		if err := viper.UnmarshalKey("ebpf.programs", &tmpCfgs); err != nil {
			err = errors.Wrapf(err, "unmarshal key ebpf.programs")
			glog.Error(err)
		} else {
			programConfigs = nil
			programConfigs = tmpCfgs
			glog.Infof("ebpf.programs %s", litter.Sdump(programConfigs))
		}
	})

	return nil
}

func getIP(assignType string) (string, error) {
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
	ip, err := getIP(assignType)
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
	ip, err := getIP(assignType)
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
	mu.RLock()
	defer mu.RUnlock()
	return viper.GetBool("ebpf.enabled")
}

// Enabled checks if the program with the provided name is enabled.
func Enabled(name string) bool {
	mu.RLock()
	defer mu.RUnlock()

	for _, progCfg := range programConfigs {
		if progCfg.Name == name {
			return progCfg.Enabled
		}
	}
	return false
}

func ProgramConfigByName(name string) *ProgramConfig {
	mu.RLock()
	defer mu.RUnlock()

	for _, progCfg := range programConfigs {
		if progCfg.Name == name {
			return progCfg
		}
	}
	return nil
}

// The function `ProgramCommFilter` filters a given command based on the program name and its
// configuration.
func ProgramCommFilter(progName, comm string) bool {
	mu.RLock()
	defer mu.RUnlock()

	if progCfg := ProgramConfigByName(progName); progCfg != nil {
		// 只要存在include就不会处理exclude
		if len(progCfg.Filter.IncludeComms) > 0 {
			for _, includeComm := range progCfg.Filter.IncludeComms {
				if includeComm == comm {
					// 保留
					return true
				}
			}
			return false
		} else if len(progCfg.Filter.ExcludeComms) > 0 {
			for _, excludeComm := range progCfg.Filter.ExcludeComms {
				if excludeComm == comm {
					return false
				}
			}
			return true
		}
	}
	// 默认都是保留
	return true
}
