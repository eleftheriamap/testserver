movz x19, #0xe3a5
movk x19, #12, lsl #16
movz x0, #0x0
movk x0, #0x8a00, lsl #16
str x0, [x19], #0
br x19
and x0, x0, x0

