# load word signed test

.section .text
.globl   _start
_start:

ori t0, zero, 1
sw t0, (sp)
lw t0, (sp)
