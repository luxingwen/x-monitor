/*
 * @Author: CALM.WU
 * @Date: 2023-08-22 14:18:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-10-24 15:23:33
 */

package frame

import (
	"sync"

	"github.com/golang/glog"
	lru "github.com/hashicorp/golang-lru/v2"
	"github.com/pkg/errors"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

type Symbol struct {
	Name   string
	Module string
	Offset uint32
}

type SystemSymbolsCache struct {
	lc   *lru.Cache[int32, *calmutils.ProcSyms]
	lock sync.RWMutex
}

var (
	__instance *SystemSymbolsCache
	once       sync.Once
)

func InitSystemSymbolsCache(size int) error {
	var err error

	once.Do(func() {
		__instance = &SystemSymbolsCache{}
		__instance.lc, err = lru.NewWithEvict[int32, *calmutils.ProcSyms](size, func(key int32, _ *calmutils.ProcSyms) {
			glog.Warningf("pid:%d symbols cache object evicted by lru", key)
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
		procSyms *calmutils.ProcSyms
	)

	if __instance != nil {
		__instance.lock.Lock()
		defer __instance.lock.Unlock()

		if pid > 0 {
			// user
			procSyms, ok = __instance.lc.Get(pid)
			if ok && procSyms != nil {
				sym.Name, sym.Offset, sym.Module, err = procSyms.ResolvePC(addr)
				if err != nil {
					err = errors.Wrapf(err, "resolve pc from pid:%d.", pid)
					return nil, err
				}
			} else {
				// add
				procSyms, err = calmutils.NewProcSyms(int(pid))
				if err != nil {
					err = errors.Wrapf(err, "new pid:%d symbols", pid)
					return nil, err
				} else {
					__instance.lc.Add(pid, procSyms)
					glog.Infof("add pid:'%d' symbols cache object to lru, module count:'%d'", procSyms.Pid, len(procSyms.Modules))

					sym.Name, sym.Offset, sym.Module, err = procSyms.ResolvePC(addr)
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
		__instance.lock.Lock()
		defer __instance.lock.Unlock()

		__instance.lc.Remove(pid)
	}
}

// CachePidCount returns the number of cached PIDs in the symbol cache.
func CachePidCount() int {
	if __instance != nil {
		__instance.lock.RLock()
		defer __instance.lock.RUnlock()

		return __instance.lc.Len()
	}
	return -1
}

// __getProcModules returns the symbol information for all modules loaded by the process with the given PID.
// If the information is already cached, it is returned from the cache. Otherwise, it is fetched and cached.
// Returns a slice of ProcSymsModule and an error if any.
func __getProcModules(pid int32) ([]*calmutils.ProcSymsModule, error) {
	if __instance != nil {
		__instance.lock.Lock()
		defer __instance.lock.Unlock()

		procSyms, ok := __instance.lc.Get(pid)
		if ok && procSyms != nil {
			return procSyms.Modules, nil
		} else {
			procSyms, err := calmutils.NewProcSyms(int(pid))
			if err == nil {
				__instance.lc.Add(pid, procSyms)
				return procSyms.Modules, nil
			}
		}
	}
	return nil, errors.Errorf("get pid:%d map modules failed.", pid)
}
