#!/bin/bash
#快速生成fstab挂载信息

if [ "$#"   -ne  2 ]; then
    echo "Usage: createfstab sda1 /media/abc"
    exit 1
fi


uuid=$(blkid | grep $1 | grep -o  " UUID=.[0-9a-zA-Z-]\\+" | sed 's/\"//g' )
type=$(blkid | grep $1 | grep -o  " TYPE=\"[0-9a-z]*"  | sed 's/TYPE=\"//g')

if [ -z "$uuid" -o -z "$type" ]; then
    echo "没有找到设备或文件系统类型"
    exit 1
fi

uuid=$(echo $uuid | sed 's/^ //g')
type=$(echo $type | sed 's/^ //g')

if [ ! -d "$2" ]; then
    mkdir -p $2
    if [ "$?" -ne 0 ]; then
        echo "创建挂载点失败"
        exit 1
    fi
fi

echo "$uuid $2 $type defaults 0 0"
