# stall caused by load
addi t0, zero, 5
sw t0, (sp)
addi t0, zero, 2
lw t1, (sp)
add t0, t1, t0
# Expected result: t0 = 7, t1 = 5
# Error case: t0 = 2, t1 = 0
