#!/bin/bash

# script for testing valgrind output.

make

rm work-at-vimeo-2.mp4

valgrind --leak-check=full --show-leak-kinds=all \
	./Main storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4

make clean
