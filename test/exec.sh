#!/bin/bash

# similar to run*.sh but without building, to better measure execution time
# does not force simple download

if [ $# -eq 1 ]
then
	threads=$1
else
	threads=4
fi

file_url=storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4
cd ../src
./Main $file_url $threads 0
