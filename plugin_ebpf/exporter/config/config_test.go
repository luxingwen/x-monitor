/*
 * @Author: CALM.WU
 * @Date: 2023-02-08 11:41:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-08 14:45:34
 */

package config

import (
	"testing"
)

func TestInitConfig(t *testing.T) {
	type args struct {
		cfgFile string
	}
	tests := []struct {
		name    string
		args    args
		wantErr bool
	}{
		{
			name:    "InitConfigTest",
			args:    args{cfgFile: "../../../env/config/xm_ebpf_plugin/config.json"},
			wantErr: false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if err := InitConfig(tt.args.cfgFile); (err != nil) != tt.wantErr {
				t.Errorf("InitConfig() error = %v, wantErr %v", err, tt.wantErr)
			}

			pprofBindAddr := GetPProfBindAddr()
			t.Logf("pprofBindAddr: %s", pprofBindAddr)
		})
	}
}
