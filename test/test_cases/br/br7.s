movz x30, #47781
movk x30, #0x2, lsl #16
movz w11, #0
movk w11, #0x8a00, lsl #16
str w11, [x30], #0
br x30
and x0, x0, x0

