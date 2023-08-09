#!/bin/bash

curl -o cpu.pprof "http://10.0.0.7:30001/debug/pprof/profile?seconds=300"
mv cpu.pprof ../../plugin_ebpf/exporter/default.pgo