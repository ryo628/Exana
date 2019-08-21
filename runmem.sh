#!/bin/sh

command="build/dynamorio/bin64/drrun -c build/lib/libmemtrace.so -- "$*
# echo $command
eval $command