/*
 * @Author: CALM.WU
 * @Date: 2023-09-04 15:21:06
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-04 17:37:00
 */

package pyrostub

import (
	"github.com/grafana/agent/component/pyroscope"
	"github.com/prometheus/client_golang/prometheus"
)

// todo init a fanOutClient

func NewPyroscopeExporter(url string, agentID string, register prometheus.Registerer) *pyroscope.Appendable {
	return nil
}
