#!/bin/bash

# Script for building and executing solution.
# forces simple download

cd ../src
make
./Main storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4 4 1
make clean
