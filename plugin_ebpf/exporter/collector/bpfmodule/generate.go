/*
 * @Author: CALM.WU
 * @Date: 2023-02-17 13:58:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-27 15:12:44
 */

package bpfmodule

//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" -type cachestat_top_statistics XMCacheStatTop ../../../bpf/xm_cachestat_top.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include
//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" -type xm_runqlat_hist XMRunQLat ../../../bpf/xm_runqlat.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include
//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" XMCacheStat ../../../bpf/xm_cachestat.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include
//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" -type xm_runqlat_hist -type xm_cpu_sched_evt_type -type xm_cpu_sched_evt_data XMCpuSchedule ../../../bpf/xm_cpu_sched.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include
//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" -type xm_processvm_evt_type -type xm_processvm_evt_data XMProcessVM ../../../bpf/xm_process_vm.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include

//go:generate genny -in=../../../../vendor/github.com/wubo0067/calmwu-go/utils/generic_channel.go -out=gen_xm_cs_sched_evt_data_channel.go -pkg=bpfmodule gen "ChannelCustomType=*XMCpuScheduleXmCpuSchedEvtData ChannelCustomName=CpuSchedEvtData"

//go:generate genny -in=../../../../vendor/github.com/wubo0067/calmwu-go/utils/generic_channel.go -out=gen_xm_process_vm_evt_data_channel.go -pkg=bpfmodule gen "ChannelCustomType=*XMProcessVMXmProcessvmEvtData ChannelCustomName=ProcessVMEvtData"
//go:generate stringer -type=XMProcessVMXmProcessvmEvtType -output=processvm_evt_type_string.go