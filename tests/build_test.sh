#!/bin/bash
PREFIX=/jp/opt/riscv-gcc-build/bin/riscv64-unknown-elf-

${PREFIX}as $1 -o $1.obj
${PREFIX}objcopy -O binary $1.obj $1.bin
rm -f $1.obj