# single forward branch test (expected misprediction)
addi t0, zero, 0
jal case5_b 
addi t0, t0, 4
case5_b:
	addi t0, t0, 1
# Expected result: t0 = 1
# If t0 = 5, pipeline is NOT getting flushed
# If t0 = 4, misprediction is not being caught at all
