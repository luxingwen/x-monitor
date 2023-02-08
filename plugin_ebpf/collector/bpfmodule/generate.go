/*
 * @Author: CALM.WU
 * @Date: 2022-12-28 11:36:20
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-28 15:29:15
 */

package bpfmodule

//go:generate env GOPACKAGE=bpfmodule bpf2go -no-strip -cc clang -type syscall_event XMTrace ../../bpf/xm_trace.bpf.c -- -I../../ -I../../bpf/.output -I../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
//go:generate env GOPACKAGE=bpfmodule bpf2go -no-strip -cc clang -type xm_runqlat_hist XMRunQLat ../../bpf/xm_runqlat.bpf.c -- -I../../ -I../../bpf/.output -I../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
//go:generate env GOPACKAGE=bpfmodule bpf2go -no-strip -cc clang -type cachestat_value XMCacheStat ../../bpf/xm_cachestat.bpf.c -- -I../../ -I../../bpf/.output -I../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
