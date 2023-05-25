movz x7, #0x5dda
movk x7, #0xb, lsl #16
movz w27, #0
movk w27, #35328, lsl #16
str w27, [x7], #0
br x7
and x0, x0, x0

