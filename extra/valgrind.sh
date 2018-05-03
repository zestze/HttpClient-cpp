#!/bin/bash

# script for testing valgrind output.

cd ../src
make
valgrind --leak-check=full --show-leak-kinds=all \
	./Main storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4 4 0
make clean
