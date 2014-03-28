#!/bin/bash 

# split a file into N pieces, each with the same number of lines

if [ $# != 2 ]; then
	/bin/echo "usage ${0} filename num_splits" >&2
	exit 1
fi

total=`/usr/bin/wc -l <"${1}"`
maxpart=`/usr/bin/expr ${2} - 1`
# /bin/echo "total lines = ${total}"
perfile=`/usr/bin/expr ${total} / ${2}`
start=1
part=0

/usr/bin/touch "${1}.0" # Always have atleast a .0 partition, if empty
while [ $start -le $total ]; do 
	end=`/usr/bin/expr ${start} + ${perfile}`
	end=`/usr/bin/expr ${end} - 1`
	if [ ${end} -gt ${total} ]; then end=${total}; fi
	if [ ${part} -eq ${maxpart} ]; then end=${total}; fi
	# /bin/echo "sed -n \"${start},${end}p\" '<'${1} '>'${1}.${part}"
	if [ ${end} -eq 0 ]; then
		/usr/bin/touch "${1}.${part}"
	else
		/bin/sed -n "${start},${end}p" <${1} >${1}.${part}
	fi
	part=`/usr/bin/expr ${part} + 1`
	start=`/usr/bin/expr ${end} + 1`
done
