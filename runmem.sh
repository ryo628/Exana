#!/bin/sh

command="build/dynamorio/bin64/drrun -c build/lib/libmemtrace.so -- "$1

# echo $command
eval $command
