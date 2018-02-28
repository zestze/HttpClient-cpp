#!/bin/bash

# similar to run*.sh but without building, to better measure execution time

file_path=storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4

cd ../src

./Main $file_path 4 1
