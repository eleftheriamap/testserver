
add x13, x2, #0x1, lsl #0
add x2, x2, #2, lsl #0
cmp x2, x13

b.ne L14
add x8, x8, #0x3a0, lsl #12
L14:
add x8, x8, #0xc3d, lsl #0
and x0, x0, x0

