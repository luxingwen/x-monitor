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
	lc *lru.Cache[uint32, *calmutils.ProcSyms]
}

var (
	__instance *SymCache
	once       sync.Once
	Size       int
)

func InitCache() error {
	var err error

	once.Do(func() {
		__instance = &SymCache{}
		__instance.lc, err = lru.NewWithEvict[uint32, *calmutils.ProcSyms](Size, func(key uint32, value *calmutils.ProcSyms) {
			glog.Warningf("pid:%d syms evicted by lru", key)
		})
		if err != nil {
			err = errors.Wrap(err, "new evict lru failed.")
		}
	})

	return err
}

func GetSymbol(pid uint32, addr uint64) *Symbol {
	if __instance != nil {
		if pid > 0 {
			// user
		} else {
			// kernel
		}
	}
	return nil
}

func RemovePidCache(pid uint32) {
	if __instance != nil && pid > 0 {
		__instance.lc.Remove(pid)
	}
}
