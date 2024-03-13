/*
 * @Author: CALM.WU
 * @Date: 2023-08-22 14:18:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-12-21 14:56:40
 */

package frame

import (
	"sync"

	"github.com/golang/glog"
	lru "github.com/hashicorp/golang-lru/v2"
	"github.com/pkg/errors"
	calmutils "github.com/wubo0067/calmwu-go/utils"
	"xmonitor.calmwu/plugin_ebpf/exporter/config"
)

type Symbol struct {
	Name   string
	Module string
	Offset uint32
}

type SystemSymbolsCache struct {
	cache *lru.Cache[int32, *calmutils.ProcMaps]
	lock  sync.RWMutex
}

var (
	__instance *SystemSymbolsCache
	once       sync.Once
)

func InitSystemSymbolsCache() error {
	var err error

	procCountLimit := config.ProcessCountLimit()
	symbolTableCountLimit := config.SymbolTableCountLimit()

	glog.Infof("procCountLimit:%d, symbolTableCountLimit:%d", procCountLimit, symbolTableCountLimit)

	once.Do(func() {
		calmutils.InitModuleSymbolTblMgr(symbolTableCountLimit)

		__instance = &SystemSymbolsCache{}
		__instance.cache, err = lru.NewWithEvict[int32, *calmutils.ProcMaps](procCountLimit, func(key int32, v *calmutils.ProcMaps) {
			glog.Warningf("pid:%d symbols cache object evicted by lru", key)
			// ** 从 symbletblMgr 中删除自己的 symbolTable，获取第一个 ProcMapsModule 的 buildID 去释放
			// 用了 LRU，就不要在对 module 引用计数了，module 数量会有一个上限
			if len(v.Modules()) > 0 {
				calmutils.DeleteModuleSymbolTbl(v.Modules()[0].BuildID)
			} else {
				glog.Warningf("pid:%d symbols cache object evicted by lru, but module count is 0", key)
			}
			v = nil
		})
		if err != nil {
			err = errors.Wrap(err, "new evict lru failed.")
		}

		if err = calmutils.LoadKallSyms(); err != nil {
			err = errors.Wrap(err, "load /proc/kallsyms failed.")
		} else {
			glog.Info("Load kernel symbols success!")
		}
	})

	return err
}

// Resolve resolves the symbol for a given process ID and address.
// It returns a pointer to a Symbol struct containing the symbol name, offset, and module (if applicable).
// If the process ID is greater than 0, it searches for user symbols. Otherwise, it searches for kernel symbols.
// If the symbol cannot be found, it logs an error and returns an empty Symbol struct.
func Resolve(pid int32, addr uint64) (*Symbol, error) {
	var (
		err      error
		ok       bool
		sym      = new(Symbol)
		procMaps *calmutils.ProcMaps
	)

	if __instance != nil {
		__instance.lock.Lock()
		defer __instance.lock.Unlock()

		if pid > 0 {
			// user, find pid symbols
			procMaps, ok = __instance.cache.Get(pid)
			if ok && procMaps != nil {
				// resolve
				sym.Name, sym.Offset, sym.Module, err = procMaps.ResolvePC(addr)
				if err != nil {
					err = errors.Wrapf(err, "resolve pc from pid:%d.", pid)
					return nil, err
				}
			} else {
				// no exist, add
				procMaps, err = calmutils.NewProcMaps(int(pid))
				if err != nil {
					err = errors.Wrapf(err, "new pid:%d symbols", pid)
					return nil, err
				} else {
					__instance.cache.Add(pid, procMaps)
					glog.Infof("add pid:'%d' symbols cache object to lru, module count:'%d'", procMaps.Pid, len(procMaps.Modules()))

					sym.Name, sym.Offset, sym.Module, err = procMaps.ResolvePC(addr)
					if err != nil {
						err = errors.Wrapf(err, "resolve pc from pid:%d.", pid)
						return nil, err
					}
				}
			}
		} else {
			// kernel
			sym.Name, err = calmutils.FindKsym(addr)
			if err != nil {
				glog.Error(err.Error())
				return nil, err
			}
		}
	}
	return sym, nil
}

// RemoveByPid removes the symbol cache for a given process ID.
func RemoveByPid(pid int32) {
	if __instance != nil && pid > 0 {
		__instance.cache.Remove(pid)
	}
}

// CachePidCount returns the number of cached PIDs in the symbol cache.
func CachePidCount() int {
	if __instance != nil {
		return __instance.cache.Len()
	}
	return -1
}

// GetProcModules retrieves the process modules for a given process ID.
// It returns a slice of *calmutils.ProcMapsModule representing the modules,
// and an error if any occurred during the retrieval process.
func GetProcModules(pid int32) ([]*calmutils.ProcMapsModule, error) {
	if __instance != nil {
		__instance.lock.Lock()
		defer __instance.lock.Unlock()

		procMaps, ok := __instance.cache.Get(pid)
		if ok && procMaps != nil {
			return procMaps.Modules(), nil
		} else {
			procMaps, err := calmutils.NewProcMaps(int(pid))
			if err == nil {
				__instance.cache.Add(pid, procMaps)
				return procMaps.Modules(), nil
			}
		}
	}
	return nil, errors.Errorf("get pid:%d map modules failed.", pid)
}

func GetLangType(pid int32) calmutils.ProcLangType {
	if __instance != nil {
		__instance.lock.Lock()
		defer __instance.lock.Unlock()

		procMaps, ok := __instance.cache.Get(pid)
		if ok && procMaps != nil {
			return procMaps.LangType
		} else {
			procMaps, err := calmutils.NewProcMaps(int(pid))
			if err == nil {
				__instance.cache.Add(pid, procMaps)
				return procMaps.LangType
			}
		}
	}
	return calmutils.NativeLangType
}
