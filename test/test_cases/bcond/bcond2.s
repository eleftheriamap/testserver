
cmp x3, x3
add x10, x3, #0x9c1, lsl #0
cmp x10, x3

b.ge L2
add x20, x20, #0xa78, lsl #0
L2:
add x20, x20, #3257, lsl #12
and x0, x0, x0

