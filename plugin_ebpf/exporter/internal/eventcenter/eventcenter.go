/*
 * @Author: CALM.WU
 * @Date: 2023-05-18 10:20:22
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-05-18 16:31:16
 */

package eventcenter

import (
	"sync"

	"github.com/golang/glog"
	"github.com/pkg/errors"
	"github.com/sourcegraph/conc"
)

// Interface for subscribing to EBPF events
type EBPFEventSubscriber interface {
	// Subscribe to a program name and one or more event types
	Subscribe(progName string, focusEvents EBPFEventType) *EBPFEventInfoChannel
}

// Interface for publishing EBPF events
type EBPFEventPublisher interface {
	// Publish an EBPFEventInfo object
	Publish(progName string, evtInfo *EBPFEventInfo) error
}

// Interface for registering and subscribing/publishing EBPF events
type EBPFEventCenterInterface interface {
	// Embed the EBPFEventSubscriber and EBPFEventPublisher interfaces
	EBPFEventSubscriber
	EBPFEventPublisher
}

var (
	_ EBPFEventCenterInterface = &EBPFEventCenter{}

	initOnce    sync.Once
	DefInstance *EBPFEventCenter
)

// eBPFEventProgram struct holds information about an event program
type eBPFEventProgram struct {
	eventReadChan *EBPFEventInfoChannel
	name          string
	focusEvents   EBPFEventType
}

type EBPFEventCenter struct {
	wg                  conc.WaitGroup
	eBPFEventProgramMap map[string]*eBPFEventProgram
	eventPublishChan    *EBPFEventInfoChannel
	stopCh              chan struct{}
	lock                sync.RWMutex
}

const (
	defaultPublishChanSize = (1 << 10)
	defaultReadChanSize    = (1 << 9)
)

func InitDefault() (EBPFEventCenterInterface, error) {
	var err error

	initOnce.Do(func() {
		ec := new(EBPFEventCenter)
		ec.eBPFEventProgramMap = make(map[string]*eBPFEventProgram)
		ec.eventPublishChan = NewEBPFEventInfoChannel(defaultPublishChanSize)
		ec.stopCh = make(chan struct{})

		ec.wg.Go(ec.DispatchEvent)

		DefInstance = ec
		glog.Info("eBPFEventCenter has been initialized.")
	})

	return DefInstance, err
}

func StopDefault() {
	if DefInstance != nil {
		DefInstance.Stop()
	}
}

func (ec *EBPFEventCenter) Stop() {
	// Close the stop channel
	close(DefInstance.stopCh)
	// Wait for the waitgroup to finish and recover from any panics
	if recover := ec.wg.WaitAndRecover(); recover != nil {
		glog.Errorf("EBPFEventCenter recover: %s", recover.String())
	}
	// Iterate over the eBPFEventProgramMap
	for _, evtProg := range ec.eBPFEventProgramMap {
		// Safely close the event read channel
		evtProg.eventReadChan.SafeClose()
	}
	// Safely close the event publish channel
	ec.eventPublishChan.SafeClose()
	// Set the eBPFEventProgramMap to nil
	ec.eBPFEventProgramMap = nil
	//
	glog.Info("eBPFEventCenter has stopped.")
}

func (ec *EBPFEventCenter) Subscribe(progName string, focusEvents EBPFEventType) *EBPFEventInfoChannel {
	ec.lock.Lock()
	defer ec.lock.Unlock()

	var evtProg *eBPFEventProgram
	var ok bool

	if evtProg, ok = ec.eBPFEventProgramMap[progName]; ok {
		glog.Infof("eBPFEventCenter eBPFProg:'%s' change focus eBPF Events from %064b ===> %064b",
			progName, evtProg.focusEvents, focusEvents)
		// 更新 evtTyes
		evtProg.focusEvents = focusEvents
	} else {
		evtProg = new(eBPFEventProgram)
		evtProg.name = progName
		evtProg.eventReadChan = NewEBPFEventInfoChannel(defaultReadChanSize)
		evtProg.focusEvents = focusEvents
		ec.eBPFEventProgramMap[progName] = evtProg
		glog.Infof("eBPFEventCenter eBPFProg:'%s' subscribe focus eBPF Events %064b", progName, focusEvents)
	}
	return evtProg.eventReadChan
}

func (ec *EBPFEventCenter) Publish(progName string, evtInfo *EBPFEventInfo) error {
	_, _, err := ec.eventPublishChan.SafeSend(evtInfo, false)
	if err != nil {
		// glog.Infof("eBPFProg:'%s' publish event:'%s' successed.", progName, evtInfo.EvtType.String())
		// return nil
		err = errors.Wrapf(err, "eBPFEventCenter eBPFProg:'%s' publish event:'%s' failed.", progName, evtInfo.EvtType.String())
		glog.Error(err)
		return err
	}
	return nil
}

func (ec *EBPFEventCenter) DispatchEvent() {
	glog.Infof("eBPFEventCenter start Pump events now....")

loop:
	for {
		select {
		case <-ec.stopCh:
			glog.Warning("eBPFEventCenter Pump events receive stop notify")
			break loop
		case evtInfo, ok := <-ec.eventPublishChan.C:
			if ok {
				ec.lock.RLock()
				for _, evtProg := range ec.eBPFEventProgramMap {
					if (evtProg.focusEvents & evtInfo.EvtType) != 0 {
						evtProg.eventReadChan.SafeSend(evtInfo, false)
					}
				}
				ec.lock.RUnlock()
			}
		}
	}
}
