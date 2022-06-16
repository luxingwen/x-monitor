#!/bin/bash
# set -e
# set -u 
# set -o pipefail

if [ ! -d "./callgrind_out" ]; then
  mkdir ./callgrind_out
fi

mv callgrind.out.* ./callgrind_out

files=$(ls ./callgrind_out/callgrind.out.*)
for file in $files;do
    gprof2dot -f callgrind -n10 -s $file > $file.dot 
    dot -Tpng $file.dot -o $file.png
done
