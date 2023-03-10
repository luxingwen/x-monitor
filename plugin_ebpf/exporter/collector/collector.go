/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:41:31
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-02 15:16:10
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
type eBPFProgram interface {
	// Get new metrics and expose them via prometheus registry.
	Update(ch chan<- prometheus.Metric) error
	Stop()
}

type eBPFProgramFactory func(string) (eBPFProgram, error)

// For a slice, users is a better name than userSlice.
var eBPFProgramRegisterMap = make(map[string]eBPFProgramFactory)

type EBPFCollector struct {
	enableCollectorDesc *prometheus.Desc
	eBPFProgramMap      map[string]eBPFProgram
}

// registerEBPFProgram registers a new eBPF sub module in the registry.
// It allows the sub module to be loaded and unloaded dynamically at runtime.
func registerEBPFProgram(name string, factory eBPFProgramFactory) {
	if _, ok := eBPFProgramRegisterMap[name]; ok {
		fmt.Printf("eBPFProgram:'%s' is already registered\n", name)
	} else {
		eBPFProgramRegisterMap[name] = factory
		fmt.Printf("eBPFProgram:'%s' is registered\n", name)
	}
}

// New creates a new eBPF collector.
// 1. It creates a new instance of the eBPF collector.
// 2. It loops through all registered eBPF modules and creates an instance of each module that is enabled.
func New() (*EBPFCollector, error) {
	eBPFCollector := &EBPFCollector{
		enableCollectorDesc: prometheus.NewDesc(
			prometheus.BuildFQName("xmonitor_eBPF", "", "enabled"),
			"Whether the x-monitor.eBPF exporter is enabled or not.",
			[]string{"enable"}, nil),
		eBPFProgramMap: make(map[string]eBPFProgram),
	}

	for progName, progFactory := range eBPFProgramRegisterMap {
		if config.Enabled(progName) {
			if eBPFProg, err := progFactory(progName); err != nil {
				err = errors.Wrapf(err, "eBPFProgram:'%s' create failed.", progName)
				glog.Error(err)
				return nil, err
			} else {
				eBPFCollector.eBPFProgramMap[progName] = eBPFProg
				glog.Infof("eBPFProgram:'%s' create success.", progName)
			}
		}
	}

	return eBPFCollector, nil
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
		for _, eBPFProg := range ec.eBPFProgramMap {
			eBPFProg.Update(ch)
		}
	} else {
		ch <- prometheus.MustNewConstMetric(ec.enableCollectorDesc, prometheus.GaugeValue, 1, "false")
	}
}

// Stop stops all the eBPF modules.
// It also unloads the eBPF programs from the kernel.
func (ec *EBPFCollector) Stop() {
	for _, eBPFProg := range ec.eBPFProgramMap {
		eBPFProg.Stop()
	}
}
