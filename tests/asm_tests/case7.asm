# single branch with stall
addi t0, zero, 0
case7_b:
	addi t0, t0, 1
	sw t0, (sp)
	lw t1, (sp)
	li t2, 7
	bne t0, t2, case7_b
	addi t0, t0, 1
# Expected values: t0 = 8, t0 = 7
