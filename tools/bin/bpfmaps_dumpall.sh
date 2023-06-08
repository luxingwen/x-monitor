#!/bin/bash

bpftool map list > data.txt
input="data.txt"

while IFS= read -r line
do
    number=$(echo "$line" | grep -o -E '^[0-9]+')
    if [[ ! -z "$number" ]]; then
        echo "dump map:""$number"
        bpftool map dump id "$number"
        echo "-------------"
    fi
done < "$input"