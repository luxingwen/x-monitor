#!/bin/bash

pidstat -r -u -t -d -w -h -p  `pidof x-monitor` 1 10000