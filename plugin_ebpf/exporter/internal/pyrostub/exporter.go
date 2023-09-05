/*
 * @Author: CALM.WU
 * @Date: 2023-09-04 15:21:06
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-05 17:20:30
 */

package pyrostub

import (
	"github.com/golang/glog"
	"github.com/grafana/agent/component/pyroscope"
	"github.com/grafana/agent/component/pyroscope/write"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
)

// todo init a fanOutClient

const (
	__defaultAgentID = "x-monitor.ebpf.instance"
)

func NewPyroscopeExporter(url string, register prometheus.Registerer) *pyroscope.Appendable {
	metrics := newMetrics(register)
	ep := write.GetDefaultEndpointOptions()
	ep.URL = url

	glog.Infof("EndpointOptions: %s", litter.Sdump(ep))

	arguments := write.DefaultArguments()
	arguments.Endpoints = []*write.EndpointOptions{&ep}

	return nil
}
