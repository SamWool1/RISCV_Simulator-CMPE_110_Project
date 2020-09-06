# AUIPC and xori test

.section .text
.globl   _start
_start:

auipc t0, 0
jal branch1
branch1:
    addi t1, ra, -4
    xor t0, t0, t1

# Expected results: t2 = 1
