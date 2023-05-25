
add x22, x8, #0x1, lsl #0
add x8, x8, #0x2, lsl #0
cmp x8, x22

b.ne L15
add x20, x20, #419, lsl #0
L15:
add x20, x20, #0x3e2, lsl #0
and x0, x0, x0

