/*
 * @Author: CALM.WU
 * @Date: 2023-05-18 11:16:50
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-05-18 11:22:26
 */

package eventcenter

//go:generate genny -in=../../../../vendor/github.com/wubo0067/calmwu-go/utils/generic_channel.go -out=gen_eventinfo_channel.go -pkg=eventcenter gen "ChannelCustomType=*EBPFEventInfo ChannelCustomName=EBPFEventInfo"
//go:generate stringer -type=EBPFEventType -output=event_type_string.go
