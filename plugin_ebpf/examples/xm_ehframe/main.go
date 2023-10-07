/*
 * @Author: CALM.WU
 * @Date: 2023-10-07 16:11:53
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-10-07 17:40:49
 */

package main

import (
	"debug/elf"
	goflag "flag"
	"os"
	"unsafe"

	"github.com/go-delve/delve/pkg/dwarf/frame"
	"github.com/golang/glog"
)

var (
	__binFile string
)

func init() {
	goflag.StringVar(&__binFile, "bin", "", "binary executable program")
}

func ptrSizeByRuntimeArch() int {
	return int(unsafe.Sizeof(uintptr(0)))
}

func main() {
	goflag.Parse()

	glog.Infof("read .eh_frame section from file:'%s'.", __binFile)

	if _, err := os.Stat(__binFile); err != nil {
		glog.Fatal(err.Error())
	}

	f, err := elf.Open(__binFile)
	//f, err := os.Open(__binFile)
	if err != nil {
		glog.Fatal(err.Error())
	}
	defer f.Close()

	section := f.Section(".eh_frame")
	data, err := section.Data()
	if err != nil {
		glog.Fatal(err.Error())
	}
	// data, err := io.ReadAll(f)
	// if err != nil {
	// 	glog.Fatal(err)
	// }

	fdes, err := frame.Parse(data, frame.DwarfEndian(data), 0, ptrSizeByRuntimeArch(), section.Addr)
	if err != nil {
		glog.Error(err.Error())
	} else {
		for _, fde := range fdes {
			glog.Infof("fde begin:0x%x, end:0x%x, length:0x%x, cie_id:0x%x, cie.Length:0x%x",
				fde.Begin(), fde.End(), fde.Length, fde.CIE.CIE_id, fde.CIE.Length)
		}
	}
}
