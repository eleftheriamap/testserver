
cmp x26, x26
add x21, x26, #0xc71, lsl #12
cmp x26, x21

b.le L5
add x19, x19, #1915, lsl #12
L5:
add x19, x19, #1409, lsl #0
and x0, x0, x0

