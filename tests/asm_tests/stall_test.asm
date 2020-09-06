# Stall test

.section .text
.globl   _start
_start:

addi t1, zero, 0
li t0, 0xDEADBEEE
sw t0, (sp)
addi t1, zero, 2
lwu t1, (sp)
addi t1, t1, 1
xor t2, t1, t0

# Expected results: t0 = 0xDEADBEEE, t1 = 0xDEADBEEF, t2 = 1
