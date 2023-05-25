#include "common/ast/reg.h"

#include "utils/log.h"


static char reg_x_strs[34][4] = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
    "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30", "xzr",
    "pc", "sp"
} ;

static char reg_w_strs[34][4] = {
    "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7",
    "w8", "w9", "w10", "w11", "w12", "w13", "w14", "w15",
    "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
    "w24", "w25", "w26", "w27", "w28", "w29", "w30", "wzr",
    "pc", "wsp"
} ;

char *__str_reg(reg_t reg) {
    if (reg.r < 0 || reg.r > 33) { log_exit_failure("Unknown register %u", reg.r) ; }
    if (reg.extended) { return reg_x_strs[reg.r] ; }
    else { return reg_w_strs[reg.r] ; }
}
