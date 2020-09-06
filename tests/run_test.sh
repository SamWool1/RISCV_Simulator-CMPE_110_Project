#!/bin/bash
../build/riscvsim << EOF 3>$1.reg
load /x 0x0 ./pt1-3-2-4-7
setptbr 0x8000
load 0x5000 $1
setpc 0x1000
run $2
test_dump_reg
EOF
