movz x29, #0x7f56
movk x29, #9, lsl #16
movz x15, #0x0
movk x15, #0x8a00, lsl #16
str x15, [x29], #0
br x29
and x0, x0, x0

