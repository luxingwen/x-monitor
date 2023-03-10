/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:43:48
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 16:49:58
 */

package collector

import (
	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/sourcegraph/conc"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"go.uber.org/atomic"
	"xmonitor.calmwu/plugin_ebpf/exporter/collector/bpfmodule"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

func init() {
	registerEBPFProgram(cacheStateProgName, newCacheStatProgram)
}

type cacheStateProgram struct {
	name        string
	stopChan    chan struct{}
	wg          conc.WaitGroup
	gatherTimer *calmutils.Timer
	// prometheus对象
	hitsDesc   *prometheus.Desc
	missesDesc *prometheus.Desc
	ratioDesc  *prometheus.Desc

	// Statistical value
	hits   atomic.Uint64
	misses atomic.Uint64
	ratio  atomic.Float64

	// eBPF对象
	objs  *bpfmodule.XMCacheStatObjects
	links []link.Link
}

func newCacheStatProgram(name string) (eBPFProgram, error) {
	csp := new(cacheStateProgram)
	csp.name = name
	csp.stopChan = make(chan struct{})
	csp.gatherTimer = calmutils.NewTimer()

	spec, err := bpfmodule.LoadXMCacheStat()
	if err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' LoadXMCacheStat failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	// set spec
	// output: cachestat.bpf.o map name:'xm_page_cache_ops_count', spec:'Hash(keySize=8, valueSize=8, maxEntries=4, flags=0)'
	for name, mapSpec := range spec.Maps {
		glog.Infof("cachestat.bpf.o map name:'%s', info:'%s', key.TypeName:'%s', value.TypeName:'%s'",
			name, mapSpec.String(), mapSpec.Key.TypeName(), mapSpec.Value.TypeName())
	}

	// cachestat.bpf.o prog name:'xm_kp_cs_atpcl', type:'Kprobe', attachType:'None', attachTo:'add_to_page_cache_lru', sectionName:'kprobe/add_to_page_cache_lru'
	for name, progSpec := range spec.Programs {
		glog.Infof("cachestat.bpf.o prog name:'%s', type:'%s', attachType:'%s', attachTo:'%s', sectionName:'%s'",
			name, progSpec.Type.String(), progSpec.AttachType.String(), progSpec.AttachTo, progSpec.SectionName)
	}

	// use spec init cacheStat object
	csp.objs = new(bpfmodule.XMCacheStatObjects)
	if err := spec.LoadAndAssign(csp.objs, nil); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' LoadAndAssign failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	if links, err := AttachObjPrograms(csp.objs.XMCacheStatPrograms, spec.Programs); err != nil {
		err = errors.Wrapf(err, "eBPFProgram:'%s' AttachObjPrograms failed.", name)
		glog.Error(err.Error())
		return nil, err
	} else {
		csp.links = links
	}

	glog.Infof("eBPFProgram:'%s' start attatchToRun successfully.", name)

	gatherInterval := config.GatherInterval(csp.name)

	// Prometheus initialization area
	csp.hitsDesc = prometheus.NewDesc(
		prometheus.BuildFQName("filesystem", "pagecache", "hits"),
		"show hits to the file system page cache",
		nil, nil,
	)
	csp.missesDesc = prometheus.NewDesc(
		prometheus.BuildFQName("filesystem", "pagecache", "misses"),
		"show misses to the file system page cache",
		nil, nil,
	)
	csp.ratioDesc = prometheus.NewDesc(
		prometheus.BuildFQName("filesystem", "pagecache", "ratio"),
		"show ratio of hits to the file system page cache",
		nil, nil,
	)

	cacheStatEventProcessor := func() {
		glog.Infof("eBPFProgram:'%s' start gathering eBPF data...", csp.name)
		// 启动定时器
		csp.gatherTimer.Reset(gatherInterval)

		var ip, count, atpcl, mpa, fad, apd, mbd uint64
		ipSlice := make([]uint64, 0, csp.objs.XMCacheStatMaps.XmPageCacheOpsCount.MaxEntries())

	loop:
		for {
			select {
			case <-csp.stopChan:
				glog.Infof("eBPFProgram:'%s' receive stop notify", csp.name)
				break loop
			case <-csp.gatherTimer.Chan():
				// glog.Infof("eBPFProgram:'%s' gather data", csp.name)

				// 迭代xm_page_cache_ops_count hash map
				entries := csp.objs.XMCacheStatMaps.XmPageCacheOpsCount.Iterate()
				for entries.Next(&ip, &count) {
					// 解析ip，判断对应的内核函数
					// glog.Infof("ip:0x%08x, count:%d", ip, count)
					if funcName, _, err := calmutils.FindKsym(ip); err == nil {
						switch {
						case funcName == "add_to_page_cache_lru":
							atpcl = count
							ipSlice = append(ipSlice, ip)
						case funcName == "mark_page_accessed":
							mpa = count
							ipSlice = append(ipSlice, ip)
						case funcName == "folio_account_dirtied":
							fad = count
							ipSlice = append(ipSlice, ip)
						case funcName == "account_page_dirtied":
							apd = count
							ipSlice = append(ipSlice, ip)
						case funcName == "mark_buffer_dirty":
							mbd = count
							ipSlice = append(ipSlice, ip)
						}
					}
				}

				if err := entries.Err(); err != nil {
					glog.Errorf("eBPFProgram:'%s' iterate map failed, err:%s", csp.name, err.Error())
				} else {
					// 开始统计计算
					total := int(mpa - mbd)
					misses := int(atpcl - apd - fad)

					if total < 0 {
						total = 0
					}
					if misses < 0 {
						misses = 0
					}
					hits := total - misses

					if hits < 0 {
						misses = total
						hits = 0
					}
					ratio := 0.0
					if total > 0 {
						ratio = float64(hits) / float64(total) * 100.0
					}

					glog.Infof("eBPFProgram:'%s' add_to_page_cache_lru:%d, mark_page_accessed:%d, "+
						"folio_account_dirtied:%d, account_page_dirtied:%d, mark_buffer_dirty:%d, "+
						"total:%d, misses:%d, >>>hits:%d, misses:%d ratio:%7.2f%%<<<",
						csp.name, atpcl, mpa, fad, apd, mbd, total, misses, hits, misses, ratio)

					csp.hits.Store(uint64(hits))
					csp.misses.Store(uint64(misses))
					csp.ratio.Store(ratio)

					// cleanup
					for _, ip := range ipSlice {
						// 清零
						err := csp.objs.XmPageCacheOpsCount.Update(ip, uint64(0), ebpf.UpdateExist)
						if err != nil {
							glog.Errorf("eBPFProgram:'%s' update map failed, err:%s", csp.name, err.Error())
						}
					}
					ipSlice = ipSlice[:0]
					atpcl = 0
					mpa = 0
					fad = 0
					apd = 0
					mbd = 0
				}

				// 重置定时器
				csp.gatherTimer.Reset(gatherInterval)
			}
		}
	}

	csp.wg.Go(cacheStatEventProcessor)

	return csp, nil
}

// Update sends the Metric values for each Metric
// associated with the cacheStateProgram to the provided channel.
// It implements prometheus.Collector.
func (csp *cacheStateProgram) Update(ch chan<- prometheus.Metric) error {
	ch <- prometheus.MustNewConstMetric(
		csp.hitsDesc, prometheus.CounterValue, float64(csp.hits.Load()))
	ch <- prometheus.MustNewConstMetric(
		csp.missesDesc, prometheus.CounterValue, float64(csp.misses.Load()))
	ch <- prometheus.MustNewConstMetric(
		csp.ratioDesc, prometheus.CounterValue, csp.ratio.Load())
	return nil
}

// Stop stops the eBPF module. This function is called when the eBPF module is removed from the system.
func (csp *cacheStateProgram) Stop() {
	close(csp.stopChan)
	if panic := csp.wg.WaitAndRecover(); panic != nil {
		glog.Errorf("eBPFProgram:'%s' panic: %v", csp.name, panic.Error())
	}

	if csp.links != nil {
		for _, link := range csp.links {
			link.Close()
		}
		csp.links = nil
	}

	if csp.objs != nil {
		csp.objs.Close()
		csp.objs = nil
	}

	if csp.gatherTimer != nil {
		csp.gatherTimer.Stop()
		csp.gatherTimer = nil
	}

	glog.Infof("eBPFProgram:'%s' stopped.", csp.name)
}
