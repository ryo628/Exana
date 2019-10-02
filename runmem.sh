#!/bin/sh

command="build/dynamorio/bin64/drrun -c build/lib/libmemtrace.so test -- "$*
# echo $command
eval $command