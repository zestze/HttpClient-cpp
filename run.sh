#!/bin/bash

make

rm work-at-vimeo-2.mp4

./Main storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4 2048 4 0

make clean
