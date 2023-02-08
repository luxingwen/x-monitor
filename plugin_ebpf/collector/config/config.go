/*
 * @Author: CALM.WU
 * @Date: 2023-02-08 11:41:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-08 14:45:34
 */

package config

import (
	"github.com/fsnotify/fsnotify"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/spf13/viper"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

type viperDebugAdapterLog struct{}

func (v *viperDebugAdapterLog) Write(p []byte) (n int, err error) {
	glog.Info(calmutils.Bytes2String(p))
	return len(p), nil
}

// InitConfig 初始化配置
func InitConfig(cfgFile string) error {
	viper.SetConfigFile(cfgFile)
	if err := viper.ReadInConfig(); err != nil {
		err = errors.Wrapf(err, "read config file %s", cfgFile)
		glog.Error(err)
		return err
	}

	viper.DebugTo(&viperDebugAdapterLog{})

	// 监控配置文件变化
	viper.WatchConfig()
	viper.OnConfigChange(func(e fsnotify.Event) {
		glog.Infof("Config file changed: %s", e.Name)
	})

	return nil
}

// GetPProfBindAddr 获取pprof绑定地址
func GetPProfBindAddr() string {
	return viper.GetString("PProf.Bind")
}
