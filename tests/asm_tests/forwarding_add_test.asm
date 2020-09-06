# Forwarding test

.section .text
.globl   _start
_start:

add t0, zero, 1
add t1, t0, t0
add t2, t1, t1
add t3, t2, t1
add t4, t3, t2
add t5, t4, t2
add t6, t5, t4
sub s0, t6, t5

# Expected results: t0 = 0x1
#		    t1 = 0x2
#		    t2 = 0x4
#		    t3 = 0x6
#		    t4 = 0xa
#		    t5 = 0xe
#		    t6 = 0x18
#		    s0 = 0xa
