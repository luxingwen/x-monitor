#!/bin/bash

valgrind --tool=callgrind --separate-threads=yes ./x-monitor -D -c ../env/config/x-monitor.cfg