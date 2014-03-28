#!/bin/bash

if [ $# != 1 ]; then
	echo "usage $0 num_lines" >&2
	exit 1
fi

typeset -i i max
max=$1
i=0

while [ $i -lt $max ]; do
	echo $RANDOM
	i=$i+1
done
