/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:41:31
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-09 16:57:12
 */

package collector

import (
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

// eBPFSubModule is the interface a collector has to implement.
type EBPFModule interface {
	// Get new metrics and expose them via prometheus registry.
	Update(ch chan<- prometheus.Metric) error
}

type eBPFModuleFactory func() (EBPFModule, error)

var __eBPFModuleRegisters = make(map[string]eBPFModuleFactory)

type EBPFCollector struct {
	enableCollectorDesc *prometheus.Desc
	eBPFModules         map[string]EBPFModule
}

// RegisterEBPFModule registers a new eBPF sub module in the registry.
// It allows the sub module to be loaded and unloaded dynamically at runtime.
func RegisterEBPFModule(name string, factory eBPFModuleFactory) {
	if _, ok := __eBPFModuleRegisters[name]; ok {
		glog.Warningf("eBPF sub module %s is already registered", name)
	} else {
		__eBPFModuleRegisters[name] = factory
	}
}

// NewEBPFCollector creates a new eBPF collector.
// 1. It creates a new instance of the eBPF collector.
// 2. It loops through all registered eBPF modules and creates an instance of each module that is enabled.
func NewEBPFCollector() (*EBPFCollector, error) {
	ebpfCollector := &EBPFCollector{
		enableCollectorDesc: prometheus.NewDesc(
			prometheus.BuildFQName("xmonitor_eBPF", "", "enabled"),
			"Whether the x-monitor.eBPF exporter is enabled or not.",
			[]string{"enable"}, nil),
		eBPFModules: make(map[string]EBPFModule),
	}

	for moduleName, moduleFactory := range __eBPFModuleRegisters {
		if config.EBPFModuleEnabled(moduleName) {
			if ebpfModule, err := moduleFactory(); err != nil {
				err = errors.Wrapf(err, "create eBPF module %s", moduleName)
				glog.Error(err)
				return nil, err
			} else {
				ebpfCollector.eBPFModules[moduleName] = ebpfModule
				glog.Infof("create eBPF module %s success", moduleName)
			}
		}
	}

	return ebpfCollector, nil
}

// Describe implements the prometheus.EBPFCollector interface.
func (ec *EBPFCollector) Describe(ch chan<- *prometheus.Desc) {
	ch <- ec.enableCollectorDesc
}

// Collect implements the prometheus.EBPFCollector interface.
func (ec *EBPFCollector) Collect(ch chan<- prometheus.Metric) {
	enabled := config.ExporterEnabled()
	if enabled {
		ch <- prometheus.MustNewConstMetric(ec.enableCollectorDesc, prometheus.GaugeValue, 1, "true")
	} else {
		ch <- prometheus.MustNewConstMetric(ec.enableCollectorDesc, prometheus.GaugeValue, 1, "false")
	}
}
