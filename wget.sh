#!/bin/bash

file_url=storage.googleapis.com/vimeo-test/work-at-vimeo-2.mp4

rm work-at-vimeo-2.mp4

wget $file_url
# @TODO: echo time
