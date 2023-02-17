/*
 * @Author: CALM.WU
 * @Date: 2023-02-17 13:58:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-17 14:00:42
 */

package bpfmodule

//go:generate env GOPACKAGE=bpfmodule bpf2go -no-strip -cc clang -type cachestat_value XMCacheStat ../../../bpf/xm_cachestat.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
//go:generate env GOPACKAGE=bpfmodule bpf2go -no-strip -cc clang -type xm_runqlat_hist XMRunQLat ../../../bpf/xm_runqlat.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
