/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:44:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-02 16:46:18
 */

package collector

import "sync"

type runqLatModule struct {
	name     string
	stopChan chan struct{}
	wg       sync.WaitGroup
	// prometheus对象
	// eBPF对象
}

func init() {}
