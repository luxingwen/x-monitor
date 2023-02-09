/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:43:48
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-09 16:35:34
 */

package cachestat

//go:generate env GOPACKAGE=cachestat bpf2go -no-strip -cc clang -type cachestat_value XMCacheStat ../../../bpf/xm_cachestat.bpf.c -- -I../../../ -I../../bpf/.output -I../../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
