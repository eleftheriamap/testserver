movz x6, #0x7066
movk x6, #4, lsl #16
movz x2, #0
movk x2, #0x8a00, lsl #16
str x2, [x6], #0
br x6
and x0, x0, x0

