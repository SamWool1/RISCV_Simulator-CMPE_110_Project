# Branch test

.section .text
.globl   _start
_start:

addi t0, zero, 0
addi t1, zero, 1
ori t2, zero, 0
jal start
loop:
	ori t2, t2, 16
start:
blt t0, t1, branch1
	ori t0, t0, 1
	xori t1, t1, 1
	ori t2, t2, 2
branch1:
bgt t1, t0, branch2
	ori t0, t0, 2
	ori t1, t1, 2
	ori t2, t2, 2
branch2:
bne t0, t1, branch3
	ori t0, t0, 4
	ori t1, t1, 4
	ori t2, t2, 4
branch3:
beq t0, zero, branch4
	ori t0, t0, 8
	ori t1, t1, 8
	ori t2, t2, 8
branch4:
beq t2, t0, loop


# Expected result: t0 = 0, t1 = 1, t2 = 0x10
