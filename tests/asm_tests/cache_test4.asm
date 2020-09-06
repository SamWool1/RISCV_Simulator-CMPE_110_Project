li t0, 0xDEADBEEF
li t1, 0xBEEF1234
li t2, 0x12345678
li t3, 0x0
addi t4, t3, 0x4
addi t5, t4, 0x4

sw t0, (t3)
sw t1, (t4)
sw t2, (t5)

lw t1, (t3)
lw t2, (t4)
lw t0, (t5)

addi t0, t0, 16
addi t1, t1, 16
addi t2, t2, 16

sw t0, (t3)
sw t1, (t4)
sw t2, (t5)

lw t2, (t3)
lw t0, (t4)
lw t1, (t5)

# Expected results:
# t0 = 0xDEADBEFF
# t1 = 0xBEEF1244
# t2 = 0x12345688
