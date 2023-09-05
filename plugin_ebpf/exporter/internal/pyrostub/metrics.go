/*
 * @Author: CALM.WU
 * @Date: 2023-09-05 17:08:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-05 17:25:15
 */

package pyrostub

import "github.com/prometheus/client_golang/prometheus"

type metrics struct {
	sentBytes       *prometheus.CounterVec
	droppedBytes    *prometheus.CounterVec
	sentProfiles    *prometheus.CounterVec
	droppedProfiles *prometheus.CounterVec
	retries         *prometheus.CounterVec
}

func newMetrics(reg prometheus.Registerer) *metrics {
	m := &metrics{
		sentBytes: prometheus.NewCounterVec(prometheus.CounterOpts{
			Name: "xmonitor_ebpf_write_sent_bytes_total",
			Help: "Total number of compressed bytes sent to x-monitor.ebpf.",
		}, []string{"endpoint"}),
		droppedBytes: prometheus.NewCounterVec(prometheus.CounterOpts{
			Name: "xmonitor_ebpf_write_dropped_bytes_total",
			Help: "Total number of compressed bytes dropped by x-monitor.ebpf.",
		}, []string{"endpoint"}),
		sentProfiles: prometheus.NewCounterVec(prometheus.CounterOpts{
			Name: "xmonitor_ebpf_write_sent_profiles_total",
			Help: "Total number of profiles sent to x-monitor.ebpf.",
		}, []string{"endpoint"}),
		droppedProfiles: prometheus.NewCounterVec(prometheus.CounterOpts{
			Name: "xmonitor_ebpf_write_dropped_profiles_total",
			Help: "Total number of profiles dropped by x-monitor.ebpf.",
		}, []string{"endpoint"}),
		retries: prometheus.NewCounterVec(prometheus.CounterOpts{
			Name: "xmonitor_ebpf_write_retries_total",
			Help: "Total number of retries to x-monitor.ebpf.",
		}, []string{"endpoint"}),
	}

	if reg != nil {
		reg.MustRegister(
			m.sentBytes,
			m.droppedBytes,
			m.sentProfiles,
			m.droppedProfiles,
			m.retries,
		)
	}

	return m
}
