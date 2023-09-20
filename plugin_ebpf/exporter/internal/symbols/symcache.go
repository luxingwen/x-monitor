/*
 * @Author: CALM.WU
 * @Date: 2023-08-22 14:18:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-22 17:20:58
 */

package symbols

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

type SymCache struct {
	lc *lru.Cache[int32, *calmutils.ProcSyms]
}

var (
	__instance *SymCache
	once       sync.Once
)

func InitCache(size int) error {
	var err error

	once.Do(func() {
		__instance = &SymCache{}
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
		if pid > 0 {
			// user
			procSyms, ok = __instance.lc.Get(pid)
			if ok && procSyms != nil {
				sym.Name, sym.Offset, sym.Module, err = procSyms.ResolvePC(addr)
				if err != nil {
					err = errors.Wrapf(err, "ResolvePC pid:%d.", pid)
					return nil, err
				}
			} else {
				// add
				procSyms, err = calmutils.NewProcSyms(int(pid))
				if err != nil {
					err = errors.Wrapf(err, "NewProcSyms pid:%d.", pid)
					return nil, err
				} else {
					__instance.lc.Add(pid, procSyms)
					glog.Infof("add pid:'%d' symbols cache object to lru, module count:'%d'", procSyms.Pid, len(procSyms.Modules))

					sym.Name, sym.Offset, sym.Module, err = procSyms.ResolvePC(addr)
					if err != nil {
						err = errors.Wrapf(err, "ResolvePC pid:%d.", pid)
						return nil, err
					}
				}
			}
		} else {
			// kernel
			sym.Name, sym.Offset, err = calmutils.FindKsym(addr)
			if err != nil {
				glog.Error(err.Error())
				return nil, err
			}
		}
	}
	return sym, nil
}

func RemoveByPid(pid int32) {
	if __instance != nil && pid > 0 {
		__instance.lc.Remove(pid)
	}
}

func Size() int {
	if __instance != nil {
		return __instance.lc.Len()
	}
	return -1
}
