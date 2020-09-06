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

# Expected results:
# t0 = 0x12345678
# t1 = 0xDEADBEEF
# t2 = 0xBEEF1234
