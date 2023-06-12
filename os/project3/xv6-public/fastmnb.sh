#!/bin/bash

make
rm fs.img
make fs.img
./xv6b.sh