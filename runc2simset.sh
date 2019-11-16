#!/bin/sh

command="./build/src/Exana -o c2sim -c set -f "$*
# echo $command
eval $command