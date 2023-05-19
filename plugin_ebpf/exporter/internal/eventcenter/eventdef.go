/*
 * @Author: CALM.WU
 * @Date: 2023-05-18 10:20:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-05-18 17:45:59
 */

package eventcenter

type EBPFEventType uint64

const (
	EBPF_EVENT_NONE EBPFEventType = iota
	EBPF_EVENT_PROCESS_EXIT
)

type EBPFEventInfo struct {
	EvtData interface{}
	EvtType EBPFEventType
}

type EBPFEventDataProcessExit struct {
	Comm string
	Pid  int32
	Tgid int32
}
