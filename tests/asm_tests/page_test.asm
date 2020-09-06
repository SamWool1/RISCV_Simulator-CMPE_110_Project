li t1, 0
li t3, 0x2500
add t4, t3, 1000
add t4, t3, 1000
add t4, t3, 1000
add t4, t3, 1000

loop_1:
	sw t1, (t3)
	addi t1, t1, 1
	addi t3, t3, 8
	bne t3, t4, loop_1

li t1, 0
li t2, 0x2500
li t2, 0

loop_2:
	lw t2, (t3)
	bne t1, t2, not_equal
	addi t1, t1, 1
	addi t3, t3, 8
	bne t3, t4, loop_2	
	jal end
	
not_equal:
	li t0, 0xDEADBEEF
end:

# Expected results: t0 == 0 (only one that matters)
