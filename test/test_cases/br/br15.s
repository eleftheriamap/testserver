movz x25, #0xfcdc
movk x25, #10, lsl #16
movz w17, #0x0
movk w17, #35328, lsl #16
str w17, [x25], #0
br x25
and x0, x0, x0

