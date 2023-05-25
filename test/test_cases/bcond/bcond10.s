
cmp w30, w30
add w16, w30, #3216, lsl #0
cmp w16, w30

b.ge L10
add w9, w9, #0xeab, lsl #0
L10:
add w9, w9, #691, lsl #12
and x0, x0, x0

