#!/bin/sh
#Loading code

sudo ./creatdevfile.sh echo
gcc -Wall test1.c -o test1
./test1 /dev/echo 
sudo ./deldevfile.sh echo

exit 0
