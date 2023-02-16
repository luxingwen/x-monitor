/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:43:48
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-10 16:38:57
 */

package cachestat

import (
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector"
)

//go:generate env GOPACKAGE=cachestat bpf2go -no-strip -cc clang -type cachestat_value XMCacheStat ../../../bpf/xm_cachestat.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB

func init() {
	collector.RegisterEBPFModule("cachestat", NewCacheStatCollector)
}

type cacheStatCollector struct{}

func NewCacheStatCollector() (collector.EBPFModule, error) {
	return nil, nil
}

func (cs *cacheStatCollector) Update(ch chan<- prometheus.Metric) error {
	return nil
}
