
add x2, x6, #1, lsl #0
add x6, x6, #0x2, lsl #0
cmp x6, x2

b.ne L12
add x21, x21, #0x362, lsl #12
L12:
add x21, x21, #2475, lsl #12
and x0, x0, x0

