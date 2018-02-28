#!/bin/bash

# similar to run*.sh but without building, to better measure execution time

threads=$1
file_url=storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4

cd ../src

./Main $file_url $threads 0
