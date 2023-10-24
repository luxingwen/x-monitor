/*
 * @Author: CALM.WU
 * @Date: 2023-03-28 18:20:50
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-18 15:25:09
 */

package utils

import (
	"unsafe"

	calmutils "github.com/wubo0067/calmwu-go/utils"
)

func CommToString(commSlice []int8) string {
	var len int

	// sh := (*reflect.StringHeader)(unsafe.Pointer(&res))
	// sh.Data = uintptr(unsafe.Pointer(&commSlice[0]))
	for _, c := range commSlice {
		if c != 0 {
			len++
		} else {
			break
		}
	}

	return unsafe.String((*byte)(unsafe.Pointer(&commSlice[0])), len)

	// byteSlice := make([]byte, 0, len(commSlice))
	// for _, v := range commSlice {
	// 	if v != 0 {
	// 		byteSlice = append(byteSlice, byte(v))
	// 	} else {
	// 		break
	// 	}
	// }
	// return calmutils.Bytes2String(byteSlice)
}

func String2Int8Array(s string, a []int8) {
	var i int
	var b byte
	bs := calmutils.String2Bytes(s)
	for i, b = range bs {
		a[i] = int8(b)
		if i >= len(a)-2 {
			break
		}
	}
	a[i+1] = 0
}
