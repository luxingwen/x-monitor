/*
 * @Author: CALM.WU
 * @Date: 2023-05-18 10:20:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-05-18 14:27:43
 */

package eventcenter

type EBPFEventType uint64

const (
	EBPF_EVENT_NONE EBPFEventType = iota
	EBPF_EVENT_PROCESS_EXIT
)

type EBPFEventInfo struct {
	EvtType EBPFEventType
	EvtData string
}

type EBPFEventDataProcessExit struct {
	pid  int32
	tgid int32
	comm string
}
