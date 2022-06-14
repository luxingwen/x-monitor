#!/bin/bash

pidstat -r -u -t -p  `pidof x-monitor` 1 10000