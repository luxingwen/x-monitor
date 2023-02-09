/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:41:31
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-09 16:57:12
 */

package collector

import "github.com/prometheus/client_golang/prometheus"

// EbpfCollector implements the prometheus.Collector interface.
type EbpfCollector struct{}

// NewEbpfCollector creates a new instance of the EbpfCollector.
func NewEbpfCollector() (*EbpfCollector, error) {
	return &EbpfCollector{}, nil
}

// Describe implements the prometheus.Collector interface.
func (ec *EbpfCollector) Describe(ch chan<- *prometheus.Desc) {
}

// Collect implements the prometheus.Collector interface.
func (ec *EbpfCollector) Collect(ch chan<- prometheus.Metric) {
}
