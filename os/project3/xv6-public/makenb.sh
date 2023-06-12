#!/bin/bash

make clean
make
make fs.img
./xv6b.sh