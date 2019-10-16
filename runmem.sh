#!/bin/sh

command="build/dynamorio/bin64/drrun -c build/lib/libmemtrace.so -- "$*
# echo $command
t1=`date +%s.%3N` 
eval $command
t2=`date +%s.%3N`
res=`echo "scale=1; ${t2}-${t1}" | bc` 
echo "execute time ${res} s"