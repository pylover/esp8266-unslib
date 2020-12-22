#! /usr/bin/env bash

make -f Makefile clean 
make -f Makefile all

if [ -z "$1" ]; then
	echo "Build Done"
elif [ $1 == "d" ]; then
	gdb $2
elif [ $1 == "a" ]; then
	make -f Makefile test
else
	./$1
fi


