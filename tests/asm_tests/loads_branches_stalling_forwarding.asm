# Tests branches, forwarding, loading, and stalls

.section .text
.globl   _start
_start:

ori t0, zero, 1
ori t1, zero, 5

branch1:
	sw t0, (sp)
	addi t0, t0, 1
	addi t2, t0, 1
	lw t3, (sp)
	add t4, t2, t3
	bne t0, t1, branch1
	
# Expected results: t0 = 5, t1 =5, t2 = 6, t3 = 4, t4 = 10
