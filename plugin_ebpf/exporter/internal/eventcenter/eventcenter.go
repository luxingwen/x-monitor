/*
 * @Author: CALM.WU
 * @Date: 2023-05-18 10:20:22
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-05-18 15:31:58
 */

package eventcenter

import (
	"sync"

	"github.com/sourcegraph/conc"
)

// Interface for subscribing to EBPF events
type EBPFEventSubscriber interface {
	// Subscribe to a program name and one or more event types
	Subscribe(progName string, evtTypes ...EBPFEventType) error
	// Get a channel of EBPFEventInfo objects
	EventChan() <-chan *EBPFEventInfo
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
	// Register a eBPFEventProgram name
	Register(progName string) error
}

var (
	_ EBPFEventCenterInterface = &EBPFEventCenter{}

	initOnce               sync.Once
	DefaultEBPFEventCenter *EBPFEventCenter
)

// eBPFEventProgram struct holds information about an event program
type eBPFEventProgram struct {
	name          string                // name of the event program
	focuseEvents  uint64                // bitmask of events to focus on
	eventReadChan *EBPFEventInfoChannel // channel for reading events
}

type EBPFEventCenter struct {
	lock                sync.Mutex
	eBPFEventProgramMap map[string]*eBPFEventProgram

	eventPublishChan *EBPFEventInfoChannel // 事件发布通道
	stopCh           chan struct{}

	wg conc.WaitGroup
}

func InitEBPfEventCenter() (EBPFEventCenterInterface, error) {
	return DefaultEBPFEventCenter, nil
}

func (ec *EBPFEventCenter) Stop() {

}

func (ec *EBPFEventCenter) Subscribe(progName string, evtTypes ...EBPFEventType) error {
	return nil
}

func (ec *EBPFEventCenter) EventChan() <-chan *EBPFEventInfo {
	return nil
}

func (ec *EBPFEventCenter) Publish(progName string, evtInfo *EBPFEventInfo) error {
	return nil
}

func (ec *EBPFEventCenter) Register(progName string) error {
	return nil
}

func (ec *EBPFEventCenter) PumpEvent() {

}
