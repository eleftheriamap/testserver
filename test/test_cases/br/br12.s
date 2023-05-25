movz x11, #0x5a79
movk x11, #14, lsl #16
movz w2, #0x0
movk w2, #35328, lsl #16
str w2, [x11], #0
br x11
and x0, x0, x0

