#!/bin/bash

vmlinux=$1
symbol=$2

if [ -z "$vmlinux" ]; then
    echo "usage : $0 vmlinux symbol"
    exit
fi

startaddress=$(nm -n $vmlinux | grep "\w\s$symbol" | awk '{print "0x"$1;exit}')
endaddress=$(nm -n $vmlinux | grep -A1 "\w\s$symbol" | awk '{getline; print "0x"$1;exit}')

if [ -z "$symbol" ]; then
    echo "dump all symbol"
    objdump -d $vmlinux
else
    echo "start-address: $startaddress, end-address: $endaddress"
    objdump -d -S $vmlinux --start-address=$startaddress --stop-address=$endaddress
fi