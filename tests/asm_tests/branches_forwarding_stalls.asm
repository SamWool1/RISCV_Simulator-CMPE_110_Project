# Tests branches, forwarding, and stalls

.section .text
.globl   _start
_start:

ori t0, zero, 0x8
sw t0, (sp)
lwu t1, (sp)
slli t1, t0, 1 # t0 = 0x10, slli
addi t3, zero, 0
branch1:
	beq t0, t1, branch2
	sw t1, (sp)
	lwu t2, (sp)
	addi t1, t1, -1
	addi t3, t3, -1
	jal branch1
branch2: 
	slli t1, t1, 1
    lui t5, 0xF23
    slli t5, t5, 8
    lui t6, 0x400
    srli t6, t6, 4
    or t5, t5, t6
	blt t3, t5, branch1
	addiw t5, zero, -1
	xor t4, t3, t5
	
# Expected results: t0 = 0x8, t1 = 0x10, t2 = 0x9, t3 = 0xfffffff8, t4 = 7

