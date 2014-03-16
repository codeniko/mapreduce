#!/bin/bash 

# split a file into N pieces, each with the same number of lines

if [ $# != 2 ]; then
	/bin/echo "usage ${0} filename num_splits" >&2
	exit 1
fi

total=`/usr/bin/wc -l <"${1}"`
maxpart=`/usr/bin/expr ${2} - 1`
echo "total lines = ${total}"
perfile=`/usr/bin/expr ${total} / ${2}`
start=1
part=0

while [ $start -le $total ]; do 
	end=`/usr/bin/expr ${start} + ${perfile}`
	end=`/usr/bin/expr ${end} - 1`
	if [ ${end} -gt ${total} ]; then end=${total}; fi
	if [ ${part} -eq ${maxpart} ]; then end=${total}; fi
	echo "sed -n \"${start},${end}p\" '<'${1} '>'${1}.${part}"
	sed -n "${start},${end}p" <${1} >${1}.${part}
	part=`/usr/bin/expr ${part} + 1`
	start=`/usr/bin/expr ${end} + 1`
done
