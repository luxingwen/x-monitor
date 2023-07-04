#!/bin/bash

# 定义一个计算函数
perform_computation() {
  local sum=0

  for ((i=0; i<1000000; i++))
  do
    sum=$(($sum + $i))
  done

  echo "Computation completed. Result: $sum"
}

# 循环调用计算函数
for ((j=1; j<=5; j++))
do
  echo "Pid: $$, Iteration $j:"
  perform_computation
  sleep 4
done

