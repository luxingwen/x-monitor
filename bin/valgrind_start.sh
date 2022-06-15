#!/bin/bash

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-leak-kinds=all ./x-monitor -D -c ../env/config/x-monitor.cfg