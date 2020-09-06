# single backward branch test
addi t0, zero, 7
case6_b:
	addi t0, t0, -1
	bne t0, zero, case6_b
# Expected results: t0 = 0
