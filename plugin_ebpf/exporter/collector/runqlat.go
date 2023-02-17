/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:44:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-17 14:38:36
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
