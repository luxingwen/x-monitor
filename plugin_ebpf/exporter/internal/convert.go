/*
 * @Author: CALM.WU
 * @Date: 2023-03-28 18:20:50
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-16 15:47:43
 */

package internal

import (
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

func CommToString(commSlice []int8) string {
	byteSlice := make([]byte, 0, len(commSlice))
	for _, v := range commSlice {
		if v != 0 {
			byteSlice = append(byteSlice, byte(v))
		} else {
			break
		}
	}
	return calmutils.Bytes2String(byteSlice)
}

// func getComm(k *profileSampleKey) string {
// 	res := ""
// 	// todo remove unsafe

// 	sh := (*reflect.StringHeader)(unsafe.Pointer(&res))
// 	sh.Data = uintptr(unsafe.Pointer(&k.Comm[0]))
// 	for _, c := range k.Comm {
// 		if c != 0 {
// 			sh.Len++
// 		} else {
// 			break
// 		}
// 	}
// 	return res
// }
