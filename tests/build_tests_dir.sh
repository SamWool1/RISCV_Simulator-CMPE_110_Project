#!/bin/bash

for file in $1/*.asm; do
    echo "building $file"
    ./build_test.sh $file
done