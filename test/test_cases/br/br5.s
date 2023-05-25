movz x10, #36110
movk x10, #15, lsl #16
movz w17, #0
movk w17, #0x8a00, lsl #16
str w17, [x10], #0
br x10
and x0, x0, x0

