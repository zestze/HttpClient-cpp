#!/bin/bash

make

rm work-at-vimeo-2.mp4

./Main storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4 4096 4 1

make clean
