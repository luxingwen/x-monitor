/*
 * @Author: CALM.WU
 * @Date: 2023-03-28 18:20:50
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-28 18:26:16
 */

package internal

import (
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

func CommToString(commSlice []int8) string {
	byteSlice := make([]byte, len(commSlice))
	for _, v := range commSlice {
		byteSlice = append(byteSlice, byte(v))
	}
	return calmutils.Bytes2String(byteSlice)
}
