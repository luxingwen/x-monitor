/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:41:31
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-09 16:57:12
 */

package collector

import (
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

// eBPFModule is the interface a collector has to implement.
type eBPFModule interface {
	// Get new metrics and expose them via prometheus registry.
	Update(ch chan<- prometheus.Metric) error
}

// Collector implements the prometheus.Collector interface.
type Collector struct {
	enableCollectorDesc *prometheus.Desc
}

// NewCollector creates a new instance of the Collector.
func NewCollector() (*Collector, error) {
	return &Collector{
		enableCollectorDesc: prometheus.NewDesc(
			prometheus.BuildFQName("xmonitor_eBPF", "", "enabled"),
			"Whether the x-monitor.eBPF exporter is enabled or not.",
			[]string{"enable"}, nil),
	}, nil
}

// Describe implements the prometheus.Collector interface.
func (ec *Collector) Describe(ch chan<- *prometheus.Desc) {
	ch <- ec.enableCollectorDesc
}

// Collect implements the prometheus.Collector interface.
func (ec *Collector) Collect(ch chan<- prometheus.Metric) {
	enabled := config.CollectorEnabled()
	if enabled {
		ch <- prometheus.MustNewConstMetric(ec.enableCollectorDesc, prometheus.GaugeValue, 1, "true")
	} else {
		ch <- prometheus.MustNewConstMetric(ec.enableCollectorDesc, prometheus.GaugeValue, 1, "false")
	}
}
