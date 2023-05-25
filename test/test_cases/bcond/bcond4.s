
add x6, x22, #0x1, lsl #0
add x22, x22, #2, lsl #0
cmp x22, x6

b.ne L4
add x0, x0, #0xba1, lsl #0
L4:
add x0, x0, #0xec5, lsl #0
and x0, x0, x0

