#!/bin/bash

for file in $1/*.asm.bin; do
    echo "running $file"
    ./run_test.sh $file 200
done