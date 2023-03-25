// This file was automatically generated by genny.
// Any changes will be lost if this file is regenerated.
// see https://github.com/cheekybits/genny

/*
 * @Author: calmwu
 * @Date: 2019-12-02 16:04:57
 * @Last Modified by: calmwu
 * @Last Modified time: 2019-12-02 16:30:21
 */

package bpfmodule

import (
	"errors"
	"fmt"
	"sync"
)

// NOTE: this is how easy it is to define a generic type

// CpuSchedEvtDataChannel channel的封装对象
type CpuSchedEvtDataChannel struct {
	C          chan *XMCpuScheduleXmCpuSchedEvtData
	mutex      sync.Mutex
	closedFlag bool
}

// NewCpuSchedEvtDataChannel 创建函数
func NewCpuSchedEvtDataChannel(size int) *CpuSchedEvtDataChannel {
	customChannel := new(CpuSchedEvtDataChannel)
	if size > 0 {
		customChannel.C = make(chan *XMCpuScheduleXmCpuSchedEvtData, size)
	} else {
		customChannel.C = make(chan *XMCpuScheduleXmCpuSchedEvtData)
	}
	customChannel.closedFlag = false
	return customChannel
}

// IsClosed 判断是否被关闭
func (cc *CpuSchedEvtDataChannel) IsClosed() bool {
	cc.mutex.Lock()
	defer cc.mutex.Unlock()
	return cc.closedFlag
}

// SafeClose 安全的关闭channel
func (cc *CpuSchedEvtDataChannel) SafeClose() {
	cc.mutex.Lock()
	defer cc.mutex.Unlock()
	if !cc.closedFlag {
		close(cc.C)
		cc.closedFlag = true
	}
}

// SafeSend 安全的发送数据
func (cc *CpuSchedEvtDataChannel) SafeSend(value *XMCpuScheduleXmCpuSchedEvtData, block bool) (ok, closed bool, err error) {
	defer func() {
		if e := recover(); e != nil {
			closed = true
			ok = false
			err = fmt.Errorf("%v", e)
		}
	}()

	err = nil

	if block {
		cc.C <- value
		ok = true
	} else {
		select {
		case cc.C <- value:
			ok = true
		default:
			ok = false
			err = errors.New("channel is full, so noblock-send failed")
		}
	}
	closed = false
	return
}

// Read 读取
func (cc *CpuSchedEvtDataChannel) Read(block bool) (val *XMCpuScheduleXmCpuSchedEvtData, ok bool) {
	if block {
		val, ok = <-cc.C
	} else {
		select {
		case val, ok = <-cc.C:
			if !ok && !cc.closedFlag {
				cc.mutex.Lock()
				defer cc.mutex.Unlock()
				cc.closedFlag = true
			}
		default:
			ok = false
		}
	}
	return
}
