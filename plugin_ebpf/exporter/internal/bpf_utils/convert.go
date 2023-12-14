/*
 * @Author: CALM.WU
 * @Date: 2023-12-13 10:21:12
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-12-13 14:49:32
 */

package bpfutils

import (
	"github.com/cilium/ebpf"
	"github.com/pkg/errors"
)

// ConvertStackID2Bytes 将堆栈 ID 转换为字节切片。
// 如果堆栈 ID 大于等于 0，则使用给定的堆栈映射查找堆栈字节。
// 如果查找失败，则返回 nil。
// 如果堆栈 ID 小于 0，则返回 nil。
func ConvertStackID2Bytes(stackID int32, stackMap *ebpf.Map) []byte {
	if stackID >= 0 {
		stackBytes, err := stackMap.LookupBytes(stackID)
		if err != nil {
			errors.Wrap(err, "stackMap.LookupBytes failed.")
			return nil
		}
		return stackBytes
	}
	return nil
}
