/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:41:31
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-17 14:35:39
 */

package collector

import (
	"fmt"

	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

// eBPFSubModule is the interface a collector has to implement.
type eBPFModule interface {
	// Get new metrics and expose them via prometheus registry.
	Update(ch chan<- prometheus.Metric) error
	Stop()
}

type eBPFModuleFactory func(string) (eBPFModule, error)

var __eBPFModuleRegisters = make(map[string]eBPFModuleFactory)

type EBPFCollector struct {
	enableCollectorDesc *prometheus.Desc
	eBPFModules         map[string]eBPFModule
}

// registerEBPFModule registers a new eBPF sub module in the registry.
// It allows the sub module to be loaded and unloaded dynamically at runtime.
func registerEBPFModule(name string, factory eBPFModuleFactory) {
	if _, ok := __eBPFModuleRegisters[name]; ok {
		fmt.Printf("eBPFModule:'%s' is already registered", name)
	} else {
		__eBPFModuleRegisters[name] = factory
		fmt.Printf("eBPFModule:'%s' is registered", name)
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
		eBPFModules: make(map[string]eBPFModule),
	}

	for moduleName, moduleFactory := range __eBPFModuleRegisters {
		if config.EBPFModuleEnabled(moduleName) {
			if ebpfModule, err := moduleFactory(moduleName); err != nil {
				err = errors.Wrapf(err, "eBPFModule:'%s' create failed.", moduleName)
				glog.Error(err)
				return nil, err
			} else {
				ebpfCollector.eBPFModules[moduleName] = ebpfModule
				glog.Infof("eBPFModule:'%s' create success.", moduleName)
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

// Stop stops all the eBPF modules.
// It also unloads the eBPF programs from the kernel.
func (ec *EBPFCollector) Stop() {
	for _, ebpfModule := range ec.eBPFModules {
		ebpfModule.Stop()
	}
}
