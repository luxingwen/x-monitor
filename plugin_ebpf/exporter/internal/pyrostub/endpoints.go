/*
 * @Author: CALM.WU
 * @Date: 2023-09-04 15:11:34
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-04 16:21:10
 */

package pyrostub

import (
	"time"

	"github.com/grafana/agent/component/common/config"
)

type EndpointOptions struct {
	Name              string
	URL               string
	RemoteTimeout     time.Duration
	Headers           map[string]string
	HTTPClientConfig  *config.HTTPClientConfig
	MinBackoff        time.Duration
	MaxBackoff        time.Duration
	MaxBackoffRetries int
}

func GetDefaultEndpointOptions() EndpointOptions {
	defaultEndpointOptions := EndpointOptions{
		RemoteTimeout:     10 * time.Second,
		MinBackoff:        500 * time.Millisecond,
		MaxBackoff:        5 * time.Minute,
		MaxBackoffRetries: 10,
		HTTPClientConfig:  config.CloneDefaultHTTPClientConfig(),
	}

	return defaultEndpointOptions
}
