
cmp x3, x3
add x30, x3, #3258, lsl #0
cmp x30, x3

b.ge L18
add x2, x2, #0x702, lsl #12
L18:
add x2, x2, #0xb98, lsl #0
and x0, x0, x0

