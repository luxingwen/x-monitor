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
	registerEBPFModule(cacheStateModuleName, newCacheStatModule)
}

type cacheStatModule struct {
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

func newCacheStatModule(name string) (eBPFModule, error) {
	csm := new(cacheStatModule)
	csm.name = name
	csm.stopChan = make(chan struct{})
	csm.gatherTimer = calmutils.NewTimer()

	spec, err := bpfmodule.LoadXMCacheStat()
	if err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' LoadXMCacheStat failed.", name)
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
	csm.objs = new(bpfmodule.XMCacheStatObjects)
	if err := spec.LoadAndAssign(csm.objs, nil); err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' LoadAndAssign failed.", name)
		glog.Error(err.Error())
		return nil, err
	}

	if links, err := AttachObjPrograms(csm.objs.XMCacheStatPrograms, spec.Programs); err != nil {
		err = errors.Wrapf(err, "eBPFModule:'%s' AttachObjPrograms failed.", name)
		glog.Error(err.Error())
		return nil, err
	} else {
		csm.links = links
	}

	gatherInterval := config.EBPFModuleInterval(csm.name)

	// Prometheus initialization area
	csm.hitsDesc = prometheus.NewDesc(
		prometheus.BuildFQName("filesystem", "pagecache", "hits"),
		"show hits to the file system page cache",
		nil, nil,
	)
	csm.missesDesc = prometheus.NewDesc(
		prometheus.BuildFQName("filesystem", "pagecache", "misses"),
		"show misses to the file system page cache",
		nil, nil,
	)
	csm.ratioDesc = prometheus.NewDesc(
		prometheus.BuildFQName("filesystem", "pagecache", "ratio"),
		"show ratio of hits to the file system page cache",
		nil, nil,
	)

	cacheStatEventProcessor := func() {
		glog.Infof("eBPFModule:'%s' start gathering eBPF data...", csm.name)
		// 启动定时器
		csm.gatherTimer.Reset(gatherInterval)

		var ip, count, atpcl, mpa, fad, apd, mbd uint64
		ipSlice := make([]uint64, 0, csm.objs.XMCacheStatMaps.XmPageCacheOpsCount.MaxEntries())

	loop:
		for {
			select {
			case <-csm.stopChan:
				glog.Infof("eBPFModule:'%s' receive stop notify", csm.name)
				break loop
			case <-csm.gatherTimer.Chan():
				// glog.Infof("eBPFModule:'%s' gather data", csm.name)

				// 迭代xm_page_cache_ops_count hash map
				entries := csm.objs.XMCacheStatMaps.XmPageCacheOpsCount.Iterate()
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
					glog.Errorf("eBPFModule:'%s' iterate map failed, err:%s", csm.name, err.Error())
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

					glog.Infof("eBPFModule:'%s' add_to_page_cache_lru:%d, mark_page_accessed:%d, "+
						"folio_account_dirtied:%d, account_page_dirtied:%d, mark_buffer_dirty:%d, "+
						"total:%d, misses:%d, >>>hits:%d, misses:%d ratio:%7.2f%%<<<",
						csm.name, atpcl, mpa, fad, apd, mbd, total, misses, hits, misses, ratio)

					csm.hits.Store(uint64(hits))
					csm.misses.Store(uint64(misses))
					csm.ratio.Store(ratio)

					// cleanup
					for _, ip := range ipSlice {
						// 清零
						err := csm.objs.XMCacheStatMaps.XmPageCacheOpsCount.Update(ip, uint64(0), ebpf.UpdateExist)
						if err != nil {
							glog.Errorf("eBPFModule:'%s' update map failed, err:%s", csm.name, err.Error())
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
				csm.gatherTimer.Reset(gatherInterval)
			}
		}
	}

	csm.wg.Go(cacheStatEventProcessor)

	return csm, nil
}

// Update sends the Metric values for each Metric
// associated with the cacheStatModule to the provided channel.
// It implements prometheus.Collector.
func (csm *cacheStatModule) Update(ch chan<- prometheus.Metric) error {
	ch <- prometheus.MustNewConstMetric(
		csm.hitsDesc, prometheus.CounterValue, float64(csm.hits.Load()))
	ch <- prometheus.MustNewConstMetric(
		csm.missesDesc, prometheus.CounterValue, float64(csm.misses.Load()))
	ch <- prometheus.MustNewConstMetric(
		csm.ratioDesc, prometheus.CounterValue, csm.ratio.Load())
	return nil
}

// Stop stops the eBPF module. This function is called when the eBPF module is removed from the system.
func (csm *cacheStatModule) Stop() {
	close(csm.stopChan)
	if panic := csm.wg.WaitAndRecover(); panic != nil {
		glog.Errorf("eBPFModule:'%s' panic: %v", csm.name, panic.Error())
	}

	if csm.links != nil {
		for _, link := range csm.links {
			link.Close()
		}
		csm.links = nil
	}

	if csm.objs != nil {
		csm.objs.Close()
		csm.objs = nil
	}

	if csm.gatherTimer != nil {
		csm.gatherTimer.Stop()
		csm.gatherTimer = nil
	}

	glog.Infof("eBPFModule:'%s' stopped.", csm.name)
}
