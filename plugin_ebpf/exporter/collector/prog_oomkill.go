/*
 * @Author: CALM.WU
 * @Date: 2023-06-14 10:31:37
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 16:13:32
 */

package collector

import (
	"bytes"
	"encoding/binary"
	"strconv"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	bpfprog "xmonitor.calmwu/plugin_ebpf/exporter/internal/bpf_prog"
	"xmonitor.calmwu/plugin_ebpf/exporter/internal/utils"
)

const (
	defaultOomKillEvtChanSize = ((1 << 2) << 10)
)

type oomKillProgram struct {
	*eBPFBaseProgram
	oomKillEvtRD       *ringbuf.Reader
	objs               *bpfmodule.XMOomKillObjects
	oomKillEvtDataChan *bpfmodule.OomkillEvtDataChannel

	sysOomKillPromDesc   *prometheus.Desc
	memcgOomKillPromDesc *prometheus.Desc
}

func init() {
	registerEBPFProgram(oomKillProgName, newOomKillProgram)
}

func loadToRunOomKillProg(name string, prog *oomKillProgram) error {
	var err error

	prog.objs = new(bpfmodule.XMOomKillObjects)
	prog.links, err = bpfprog.AttachToRun(name, prog.objs, bpfmodule.LoadXMOomKill, func(spec *ebpf.CollectionSpec) error {
		return nil
	})

	if err != nil {
		prog.objs.Close()
		prog.objs = nil
		return err
	}

	prog.oomKillEvtRD, err = ringbuf.NewReader(prog.objs.XmOomkillEventRingbufMap)
	if err != nil {
		for _, link := range prog.links {
			link.Close()
		}
		prog.links = nil
		prog.objs.Close()
		prog.objs = nil
	}
	return err
}

func newOomKillProgram(name string) (eBPFProgram, error) {
	prog := &oomKillProgram{
		eBPFBaseProgram: &eBPFBaseProgram{
			name:     name,
			stopChan: make(chan struct{}),
		},
		oomKillEvtDataChan: bpfmodule.NewOomkillEvtDataChannel(defaultOomKillEvtChanSize),
		sysOomKillPromDesc: prometheus.NewDesc("oom_kill_os", "Details of the process when it is oom killed in the system",
			[]string{"pid", "tid", "comm", "file_rss_kb", "anon_rss_kb", "shmem_rss_kb", "mem_limit_kb", "msg"},
			prometheus.Labels{"from": "xm_ebpf"}),
		memcgOomKillPromDesc: prometheus.NewDesc("oom_kill_memcg", "Details of the process when it is oom killed in the memory cgroup",
			[]string{"pid", "tid", "comm", "memcg_inode", "memcg_limit_kb", "file_rss_kb", "anon_rss_kb", "shmem_rss_kb", "msg"},
			prometheus.Labels{"from": "xm_ebpf"}),
	}

	if err := loadToRunOomKillProg(name, prog); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' loadToRunOomKillProg failed.", name)
		glog.Error(err)
		return nil, err
	}

	prog.wg.Go(prog.tracingBPFEvent)
	return prog, nil
}

func (okp *oomKillProgram) tracingBPFEvent() {
	glog.Infof("eBPFProgram:'%s' start tracing eBPF Event...", okp.name)

loop:
	for {
		record, err := okp.oomKillEvtRD.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				glog.Warningf("eBPFProgram:'%s' tracing eBPF Event goroutine receive stop notify", okp.name)
				break loop
			}
			glog.Errorf("eBPFProgram:'%s' Read error. err:%s", okp.name, err.Error())
			continue
		}
		oomKillEvtData := new(bpfmodule.XMOomKillXmOomkillEvtData)
		if err := binary.Read(bytes.NewBuffer(record.RawSample), binary.LittleEndian, oomKillEvtData); err != nil {
			glog.Errorf("failed to parse XMOomKillXmOomkillEvtData, err: %s", err.Error())
			continue
		}
		okp.oomKillEvtDataChan.SafeSend(oomKillEvtData, false)
	}
}

func (okp *oomKillProgram) Update(ch chan<- prometheus.Metric) error {
	glog.Infof("eBPFProgram:'%s' update start...", okp.name)
	defer glog.Infof("eBPFProgram:'%s' update done.", okp.name)

loop:
	for {
		select {
		case data, ok := <-okp.oomKillEvtDataChan.C:
			if ok {
				comm := utils.CommToString(data.Comm[:])
				oomMsg := utils.CommToString(data.Msg[:])
				fileRssKB := data.RssFilepages << 2
				anonRssKB := data.RssAnonpages << 2
				shmemRssKB := data.RssShmepages << 2
				memoryLimitKB := data.TotalPages << 2
				if data.MemcgId != 0 {
					// memcgid就是inode，通过类似命令可以找到对应的cg目录，find / -inum 140451 -print
					memcgKB := data.MemcgPageCounter << 2
					glog.Infof("eBPFProgram:'%s' Process pid:%d, tid:%d, comm:'%s' happens OOMKill, in MemCgroup:%d, MemCgroup alloc:%dkB, badness points:%d, file-rss:%dkB, anon-rss:%dkB, shmem-rss:%dkB, memoryLimit:%dkB, Msg:%s",
						okp.name, data.Pid, data.Tid, comm, data.MemcgId, memcgKB, data.Points, fileRssKB, anonRssKB, shmemRssKB, memoryLimitKB, oomMsg)

					ch <- prometheus.MustNewConstMetric(okp.memcgOomKillPromDesc,
						prometheus.GaugeValue, float64(data.Points),
						strconv.FormatInt(int64(data.Pid), 10),
						strconv.FormatInt(int64(data.Tid), 10),
						comm, strconv.FormatInt(int64(data.MemcgId), 10),
						strconv.FormatInt(int64(memcgKB), 10),
						strconv.FormatInt(int64(fileRssKB), 10),
						strconv.FormatInt(int64(anonRssKB), 10),
						strconv.FormatInt(int64(shmemRssKB), 10),
						oomMsg)
				} else {
					glog.Infof("eBPFProgram:'%s' Process pid:%d, tid:%d, comm:'%s' happens OOMKill, badness points:%d, file-rss:%dkB, anon-rss:%dkB, shmem-rss:%dkB, memoryLimit:%dkB, Msg:%s",
						okp.name, data.Pid, data.Tid, comm, data.Points, fileRssKB, anonRssKB, shmemRssKB, memoryLimitKB, oomMsg)

					ch <- prometheus.MustNewConstMetric(okp.sysOomKillPromDesc,
						prometheus.GaugeValue, float64(data.Points),
						strconv.FormatInt(int64(data.Pid), 10),
						strconv.FormatInt(int64(data.Tid), 10),
						comm,
						strconv.FormatInt(int64(fileRssKB), 10),
						strconv.FormatInt(int64(anonRssKB), 10),
						strconv.FormatInt(int64(shmemRssKB), 10),
						strconv.FormatInt(int64(memoryLimitKB), 10),
						oomMsg)
				}
			}
		default:
			break loop
		}
	}
	return nil
}

func (okp *oomKillProgram) Stop() {
	if okp.oomKillEvtRD != nil {
		okp.oomKillEvtRD.Close()
	}

	okp.finalizer()

	if okp.objs != nil {
		okp.objs.Close()
		okp.objs = nil
	}

	okp.oomKillEvtDataChan.SafeClose()
	glog.Infof("eBPFProgram:'%s' stopped", okp.name)
}

/*
I0615 07:28:43.679520 1284119 prog_oomkill.go:126] eBPFProgram:'oomkill' Process pid:1284158, tid:1284158, comm:'stress-ng-vm' happens OOMKill, in MemCgroup:140451, MemCgroup pageCounter:262144, badness points:524271, file-rss:1372kB, anon-rss:1045532kB, shmem-rss:36kB, memoryLimit:1048576kB, Msg:Memory cgroup out of memory

➜  pingan find  / -inum 140451 -print
/sys/fs/cgroup/memory/user.slice/hello-cg


[3020572.724656] [1284158]     0 1284158   282787   261735  2195456        0          1000 stress-ng-vm
[3020572.729020] oom-kill:constraint=CONSTRAINT_MEMCG,nodemask=(null),cpuset=/,mems_allowed=0,oom_memcg=/user.slice/hello-cg,task_memcg=/user.slice/hello-cg,task=stress-ng-vm,pid=1284158,uid=0
[3020572.737251] Memory cgroup out of memory: Killed process 1284158 (stress-ng-vm) total-vm:1131148kB, anon-rss:1045532kB, file-rss:1372kB, shmem-rss:36kB, UID:0 pgtables:2144kB oom_score_adj:1000
[3020572.748425] oom_reaper: reaped process 1284158 (stress-ng-vm), now anon-rss:0kB, file-rss:0kB, shmem-rss:36kB


cgget -g memory:/user.slice/hello-cg
memory.kmem.slabinfo:
memory.limit_in_bytes: 1073741824 = 1048576kB
memory.swappiness: 60
*/
