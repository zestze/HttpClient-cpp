#!/bin/bash

# Script for building and executing the solution
# does not force simple download

if [ $# -eq 1 ]
then
	args=$1
else
	args=4
fi

cd ../src
make
./Main storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4 $args 0
make clean
