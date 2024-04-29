/*
 * @Author: CALM.WU
 * @Date: 2024-04-29 15:30:13
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-29 16:13:58
 */

package main

import (
	"flag"

	"github.com/golang/glog"
)

const (
	upStairsStepCount1 = 2
	upStairsStepCount2 = 3
)

var (
	totalStep int
)

func init() {
	flag.IntVar(&totalStep, "total", 20, "total steps")
}

func climbStairs(remainderStep int) int {
	if remainderStep == 0 {
		return 1
	}
	if remainderStep < 0 {
		return 0
	}
	return climbStairs(remainderStep-upStairsStepCount1) + climbStairs(remainderStep-upStairsStepCount2)
}

func main() {
	flag.Parse()

	glog.Info("start climb stairs")
	count := climbStairs(totalStep)
	glog.Infof("total climb stairs count: %d", count)
}

// ./main.exe --total=8  --alsologtostderr -v=4
