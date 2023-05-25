movz x14, #0x85e2
movk x14, #0xc, lsl #16
movz x2, #0
movk x2, #35328, lsl #16
str x2, [x14], #0
br x14
and x0, x0, x0

