movz x25, #0x3540
movk x25, #0x9, lsl #16
movz x5, #0x0
movk x5, #35328, lsl #16
str x5, [x25], #0
br x25
and x0, x0, x0

