#!/bin/bash

valgrind --tool=memcheck --leak-check=yes ./x-monitor -D -c ../env/config/x-monitor.cfg