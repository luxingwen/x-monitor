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

// BioRequestLatencyEvtDataChannel channel的封装对象
type BioRequestLatencyEvtDataChannel struct {
	C          chan *XMBioXmBioRequestLatencyEvtData
	mutex      sync.Mutex
	closedFlag bool
}

// NewBioRequestLatencyEvtDataChannel 创建函数
func NewBioRequestLatencyEvtDataChannel(size int) *BioRequestLatencyEvtDataChannel {
	customChannel := new(BioRequestLatencyEvtDataChannel)
	if size > 0 {
		customChannel.C = make(chan *XMBioXmBioRequestLatencyEvtData, size)
	} else {
		customChannel.C = make(chan *XMBioXmBioRequestLatencyEvtData)
	}
	customChannel.closedFlag = false
	return customChannel
}

// IsClosed 判断是否被关闭
func (cc *BioRequestLatencyEvtDataChannel) IsClosed() bool {
	cc.mutex.Lock()
	defer cc.mutex.Unlock()
	return cc.closedFlag
}

// SafeClose 安全的关闭channel
func (cc *BioRequestLatencyEvtDataChannel) SafeClose() {
	cc.mutex.Lock()
	defer cc.mutex.Unlock()
	if !cc.closedFlag {
		close(cc.C)
		cc.closedFlag = true
	}
}

// SafeSend 安全的发送数据
func (cc *BioRequestLatencyEvtDataChannel) SafeSend(value *XMBioXmBioRequestLatencyEvtData, block bool) (ok, closed bool, err error) {
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
func (cc *BioRequestLatencyEvtDataChannel) Read(block bool) (val *XMBioXmBioRequestLatencyEvtData, ok bool) {
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
