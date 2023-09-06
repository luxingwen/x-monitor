/*
 * @Author: CALM.WU
 * @Date: 2023-09-04 15:21:06
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-06 15:19:25
 */

package pyrostub

import (
	"fmt"

	"github.com/go-kit/log"
	"github.com/golang/glog"
	"github.com/grafana/agent/component"
	"github.com/grafana/agent/component/pyroscope"
	"github.com/grafana/agent/component/pyroscope/write"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sanity-io/litter"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

// todo init a fanOutClient

const (
	__defaultAgentIDFmt = "x-monitor.ebpf.%s"
)

func NewPyroscopeExporter(url string, register prometheus.Registerer) (pyroscope.Appendable, error) {
	var (
		appender pyroscope.Appendable
		err      error
	)

	ip, _ := config.APISrvBindAddr()
	agentID := fmt.Sprintf(__defaultAgentIDFmt, ip)

	// make args
	ep := write.GetDefaultEndpointOptions()
	ep.URL = url
	glog.Infof("agentID:'%s', EndpointOptions: %s", agentID, litter.Sdump(ep))
	args := write.DefaultArguments()
	args.Endpoints = []*write.EndpointOptions{&ep}

	// make options
	options := component.Options{
		ID:         agentID,
		Logger:     log.With(__defaultLogger, "component", "pyrostub"),
		Registerer: register,
		OnStateChange: func(e component.Exports) {
			appender = e.(write.Exports).Receiver
			glog.Infof("agentID:'%s', set appender", agentID)
		},
	}

	if _, err = write.New(options, args); err != nil {
		return nil, errors.Wrapf(err, "agentID:'%s' new write component failed.", agentID)
	}
	return appender, nil
}
