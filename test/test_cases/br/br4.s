movz x10, #0x4db6
movk x10, #9, lsl #16
movz w30, #0x0
movk w30, #0x8a00, lsl #16
str w30, [x10], #0
br x10
and x0, x0, x0

