#!/bin/bash

# Script for testing multiple thread pool sizes

file="../log.txt"
[ -e $file ] && rm $file

cd ../src
make
cd ../test

printf "wget time: " >> $file
(time ./wget.sh) 2>> $file
printf "wget time: " >> $file
(time ./wget.sh) 2>> $file
printf "wget time: " >> $file
(time ./wget.sh) 2>> $file

printf "\nsimple download time: " >> $file
(time ./exec-simple.sh) 2>> $file
printf "\nsimple download time: " >> $file
(time ./exec-simple.sh) 2>> $file
printf "\nsimple download time: " >> $file
(time ./exec-simple.sh) 2>> $file

for i in `seq 2 16`;
do
	printf "\n$i threads download time: " >> $file
	(time ./exec.sh $i) 2>> $file
	printf "\n$i threads download time: " >> $file
	(time ./exec.sh $i) 2>> $file
	printf "\n$i threads download time: " >> $file
	(time ./exec.sh $i) 2>> $file
done

cd ../src
make clean
