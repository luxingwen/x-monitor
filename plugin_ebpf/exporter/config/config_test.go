/*
 * @Author: CALM.WU
 * @Date: 2023-02-08 11:41:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-10 14:51:50
 */

package config

import (
	"testing"

	"github.com/vishvananda/netlink"
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
			args:    args{cfgFile: "../../../env/config/xm_ebpf_plugin/config.yaml"},
			wantErr: false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if err := InitConfig(tt.args.cfgFile); (err != nil) != tt.wantErr {
				t.Errorf("InitConfig() error = %v, wantErr %v", err, tt.wantErr)
			}

			pprofBindAddr, err := GetPProfBindAddr()
			if err != nil {
				t.Errorf("GetPProfBindAddr() error = %v", err)
			} else {
				t.Logf("pprofBindAddr: %s", pprofBindAddr)
			}
		})
	}
}

func Test__getIP(t *testing.T) {
	type args struct {
		assignType string
	}
	tests := []struct {
		name    string
		args    args
		want    string
		wantErr bool
	}{
		{
			name: "route_list",
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			routeList, err := netlink.RouteList(nil, netlink.FAMILY_V4)
			if err != nil {
				t.Errorf("netlink.RouteList error = %v", err)
			} else {
				for i, route := range routeList {
					// if route.Dst == nil {
					// 	return route.Gw.String(), nil
					// }
					t.Logf("%d, route: %s", i, route.String())
					link, err := netlink.LinkByIndex(route.LinkIndex)
					if err == nil {
						t.Logf("link, type:'%s', name:'%s'", link.Type(), link.Attrs().Name)
					} else {
						t.Errorf("netlink.LinkByIndex error = %v", err)
					}

					addrs, err := netlink.AddrList(link, netlink.FAMILY_V4)
					if err == nil {
						for _, addr := range addrs {
							// ens160, addr: 192.168.14.128/24 ens160, ip: 192.168.14.128
							t.Logf("%s, addr: %s, ip: %s", link.Attrs().Name, addr.String(), addr.IP.String())
						}
					} else {
						t.Errorf("netlink.AddrList error = %v", err)
					}
				}
			}
		})
	}
}
