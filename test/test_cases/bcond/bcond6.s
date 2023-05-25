
add x19, x6, #0xf0d, lsl #0
cmp x19, x6

b.gt L6
add x17, x17, #352, lsl #0
L6:
add x17, x17, #0xa2a, lsl #12
and x0, x0, x0

