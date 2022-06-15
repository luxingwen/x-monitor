#!/bin/bash

valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./x-monitor -D -c ../env/config/x-monitor.cfg