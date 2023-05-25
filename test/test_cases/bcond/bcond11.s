
add x25, x17, #0x1, lsl #0
add x17, x17, #2, lsl #0
cmp x17, x25

b.ne L11
add x10, x10, #1512, lsl #0
L11:
add x10, x10, #3809, lsl #12
and x0, x0, x0

