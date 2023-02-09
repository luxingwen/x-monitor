/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 14:44:25
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-09 16:36:44
 */

package runqlat

//go:generate env GOPACKAGE=runqlat bpf2go -no-strip -cc clang -type xm_runqlat_hist XMRunQLat ../../../bpf/xm_runqlat.bpf.c -- -I../../../ -I../../bpf/.output -I../../../../extra/include/bpf -g -Wall -Werror -D__TARGET_ARCH_x86 -DOUTPUT_SKB
