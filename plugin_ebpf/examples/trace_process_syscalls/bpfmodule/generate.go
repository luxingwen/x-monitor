/*
 * @Author: CALM.WU
 * @Date: 2022-12-28 11:36:20
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-28 15:29:15
 */

package bpfmodule

//go:generate env GOPACKAGE=bpfmodule bpf2go -no-strip -cc clang TraceProcessSyscalls ../../../bpf/xm_trace_syscalls.bpf.c -- -I../../../ -I../../../bpf/.output -I../../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
