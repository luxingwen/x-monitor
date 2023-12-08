#!/bin/bash

# 接收循环次数参数
times=$1

# 请求URL
url="localhost:8000"

# 循环发送请求
for ((i=0; i<$times; i++)); do
  echo "发送请求 $i"
  curl "$url"
done
