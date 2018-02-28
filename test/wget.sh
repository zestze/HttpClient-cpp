#!/bin/bash

file_url=storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4
file=work-at-vimeo-2.mp4
[ -e $file ] && rm $file

wget --quiet $file_url
