movz x22, #0x3dac
movk x22, #0x5, lsl #16
movz w10, #0
movk w10, #35328, lsl #16
str w10, [x22], #0
br x22
and x0, x0, x0

