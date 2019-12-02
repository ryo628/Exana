#!/bin/sh

#DYNAMORIO_HOME="$(cd $(dirname $0) && pwd)/dynamorio"
#eval "export DYNAMORIO_HOME=${DYNAMORIO_HOME}"

eval "rm ./tmp/*"
t1=`date +%s.%3N`
command="build/dynamorio/bin64/drrun -c build/lib/libmemtrace.so -- "$*
# echo $command
eval $command
t2=`date +%s.%3N`
res=`echo "scale=1; ${t2}-${t1}" | bc`
echo "execute time ${res} s"