/*
 * @Author: CALM.WU
 * @Date: 2024-03-07 11:06:41
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2024-03-07 11:06:41
 */

package collector

import (
	"time"

	"github.com/golang/glog"
	"github.com/mitchellh/mapstructure"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/bpfutil"
)

type nfsProgPrivateArgs struct {
	GatherInterval time.Duration `mapstructure:"gather_interval"`
}

type nfsProgram struct {
	*eBPFBaseProgram
	objs                     *bpfmodule.XMNfsObjects
	nfsOperLatencyMetricDesc *prometheus.Desc
}

var (
	__nfsProgPrivateArgs nfsProgPrivateArgs
)

func init() {
	registerEBPFProgram(nfsProgName, newNFSProgram)
}

func loadToRunNFSProg(name string, prog *nfsProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMNfsObjects)
	prog.links, err = bpfutil.AttachToRun(name, prog.objs, bpfmodule.LoadXMNfs, nil)

	if err != nil {
		prog.objs.Close()
		prog.objs = nil
		return err
	}
	return nil
}

func newNFSProgram(name string) (eBPFProgram, error) {
	mapstructure.Decode(config.ProgramConfigByName(name).Args.Private, &__nfsProgPrivateArgs)
	__nfsProgPrivateArgs.GatherInterval = __nfsProgPrivateArgs.GatherInterval * time.Second

	nfsProg := &nfsProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:        name,
			stopChan:    make(chan struct{}),
			gatherTimer: calmutils.NewTimer(),
		},
		nfsOperLatencyMetricDesc: prometheus.NewDesc(
			prometheus.BuildFQName("fs", "nfs", "operation_latency"),
			"Latency of NFS operations, shows this time as a histogram. in microseconds.",
			[]string{"device", "operation"}, prometheus.Labels{"from": "xm_ebpf"}),
	}

	if err := loadToRunNFSProg(name, nfsProg); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' loadToRunNFSProg failed.", name)
		glog.Error(err)
		return nil, err
	}

	return nfsProg, nil
}

func (np *nfsProgram) Update(ch chan<- prometheus.Metric) error {
	return nil
}

func (np *nfsProgram) Stop() {
	np.finalizer()

	if np.objs != nil {
		np.objs.Close()
		np.objs = nil
	}
	glog.Infof("eBPFProgram:'%s' stopped.", np.name)
}

func (np *nfsProgram) Reload() error {
	return nil
}
