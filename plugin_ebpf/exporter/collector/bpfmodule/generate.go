/*
 * @Author: CALM.WU
 * @Date: 2023-02-17 13:58:55
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 16:07:34
 */

package bpfmodule

//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" -type cachestat_value XMCacheStatTop ../../../bpf/xm_cachestat_top.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include/bpf
//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" -type xm_runqlat_hist XMRunQLat ../../../bpf/xm_runqlat.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include/bpf
//go:generate env GOPACKAGE=bpfmodule bpf2go -cc clang -cflags "-g -O2 -Wall -Werror -Wno-unused-function -D__TARGET_ARCH_x86 -DOUTPUT_SKB" XMCacheStat ../../../bpf/xm_cachestat.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include/bpf
