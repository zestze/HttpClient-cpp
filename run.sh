#!/bin/bash

make

rm work-at-vimeo-2.mp4

./main storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4

make clean
