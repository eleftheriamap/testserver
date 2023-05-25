
add x26, x11, #0x1, lsl #0
add x11, x11, #0x2, lsl #0
cmp x11, x26

b.ne L17
add x22, x22, #1406, lsl #0
L17:
add x22, x22, #3833, lsl #0
and x0, x0, x0

