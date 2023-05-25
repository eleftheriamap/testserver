#include <stdint.h>

#include "assembler/encoder.h"
#include "utils/bits.h"
#include "common/word.h"


// 0x4 = 0b100 100?
#define OP0_INSTR_DP (0x4 << 26)
// 0x5 = 0b101 (bit 25), ?101
#define OP0_INSTR_DP_REG (0x5 << 25)
#define OP0_INSTR_BRANCH (0x5 << 26)
#define OP0_INSTR_LDST ((1 << 27) | (0 << 25))

// 0x2 = 0b010
#define DP_IMM_ADDSUB_OP0 (0x2 << 23)
#define DP_IMM_LOG_OP0 (0x4 << 22) 
#define DP_IMM_MOV_W_OP0 (0x5 << 23)
//                        ---op1--- | --------op2--------- 
#define DP_SH_ADDSUB_OPS ((0 << 28) | (1 << 24) | (0 << 21))
#define DP_SH_LOG_OP1 (0 << 28)
#define DP_SH_LOG_OP2 (0 << 24)

uint32_t bitmask(int high_bit, int low_bit) {
    ASSERT(high_bit >= low_bit && high_bit <= 32 && low_bit >= 0) ;
    uint64_t mask = 0 ;
    int mask_length = high_bit - low_bit ;
    mask = (1 << mask_length) - 1 ;
    return mask ;
}

/*****************************************************************************/
// DP Immediate

/// @brief Flatten encoded add imm argument.
void to_word_add_imm(word32_t *dest, enc_add_imm i) {
    set_bit_at(dest, 30, i.is_subtract) ;
    set_bit_at(dest, 29, i.set_cond_flags) ;
    set_bits_at(dest, 23, 0b010) ;
    set_bit_at(dest, 22, i.shift_imm) ;
    set_bits_at(dest, 10, i.imm12) ;
    set_bits_at(dest, 5, i.xn) ;
}

/// @brief Flatten encoded mov(n|z|k) instruction.
void to_word_mov(word32_t *dest, enc_mov i) {
    set_bits_at(dest, 29, i.op_tp) ;
    set_bits_at(dest, 23, 0b101) ;
    set_bits_at(dest, 21, i.shift) ;
    set_bits_at(dest, 5, i.imm16) ;
    set_bits_at(dest, 0, i.xd) ;
}

/*****************************************************************************/
// DP Registers

/// @brief Flatten encoded add reg_t argument.
void to_word_add_reg(word32_t *dest, enc_add_reg i) {
    set_bit_at(dest, 30, i.is_subtract) ;
    set_bit_at(dest, 29, i.set_cond_flags) ;
    set_bits_at(dest, 24, 0b01011) ;
    set_bits_at(dest, 22, i.shift_type) ;
    set_bit_at(dest, 21, 0) ;
    set_bits_at(dest, 10, i.shift_amount) ;
}

/// @brief Flatten encoded logical reg_t argument.
void to_word_log_reg(word32_t *dest, enc_log_reg i) {
    set_bits_at(dest, 29, i.opc) ;
    set_bits_at(dest, 24, 0b01010) ;
    set_bits_at(dest, 22, i.shift_type) ;
    set_bit_at(dest, 21, i.negate) ;
    set_bits_at(dest, 10, i.shift_amount) ;
}

/// @brief Flatten encoded mul.
void to_word_mul(word32_t *dest, enc_mul i) {
    set_bit_at(dest, 28, 1) ;
    set_bit_at(dest, 24, 1) ;

    set_bits_at(dest, 29, 0b00) ;
    set_bits_at(dest, 21, 0b000) ;

    set_bit_at(dest, 15, i.is_negate) ;
    set_bits_at(dest, 10, i.xa) ;
}

/*****************************************************************************/
// Branches

/// @brief Flatten encoded conditional branch instruction.
void to_word_b_cond(word32_t *dest, enc_b_cond i) {
    set_bits_at(dest, 26, 0b10101) ;
    set_bits_at(dest, 5, i.imm19) ;
    set_bits_at(dest, 0, i.cond) ;
}

/// @brief Flatten encoded unconditional register branch instruction.
void to_word_b_reg(word32_t *dest, enc_b_reg i) {
    set_bits_at(dest, 25, 0b1101011) ;
    set_bits_at(dest, 16, 0b11111) ;
    set_bits_at(dest, 5, i.xn) ;
}

/// @brief Flatten encoded unconditional immediate branch instruction.
void to_word_b_imm(word32_t *dest, enc_b_imm i) {
    set_bits_at(dest, 26, 0b101) ;
    set_bits_at(dest, 0, i.imm26) ;
}

/*****************************************************************************/
// Instr Types

/// @brief Flatten encoded dp_imm instruction.
void to_word_dp_imm(word32_t *dest, enc_dp_imm i) {
    set_bit_at(dest, 31, i.sf) ;
    set_bits_at(dest, 26, 0b100) ;
    set_bits_at(dest, 0, i.xd) ;
    switch (i.tp)
    {
        case DP_ADD: to_word_add_imm(dest, i.add_imm) ; break ;
        case DP_LOG:
            not_implemented_error("to_word_dp_imm DP_LOG") ;
        case DP_MOV: to_word_mov(dest, i.mov) ; break ;
        case DP_MUL:
            log_exit_failure("Internal Error: to_word_dp_imm called on DP_MUL") ;
    }
}

/// @brief Flatten encoded dp_reg instruction.
void to_word_dp_reg(word32_t *dest, enc_dp_reg i) {
    loglvl(LOG_0, "sf: %d\n", i.sf) ;
    set_bit_at(dest, 31, i.sf) ;
    set_bits_at(dest, 25, 0b101) ;
    set_bits_at(dest, 16, i.xm) ;
    set_bits_at(dest, 5, i.xn) ;
    set_bits_at(dest, 0, i.xd) ;

    switch (i.tp)
    {
        case DP_ADD: to_word_add_reg(dest, i.add_reg) ; break ;
        case DP_LOG: to_word_log_reg(dest, i.log_reg) ; break ;
        case DP_MUL: to_word_mul(dest, i.mul) ; break ;
        case DP_MOV: log_exit_failure("to_word_dp_reg DP_MOV") ; break ;
    }
}

/// @brief Flatten encoded branch instruction.
void to_word_branch(word32_t *dest, enc_branch i) {
    switch (i.tp){
        case E_B_IMM: to_word_b_imm(dest, i.imm) ; break ;
        case E_B_REG: to_word_b_reg(dest, i.reg) ; break ;
        case E_B_COND: to_word_b_cond(dest, i.cond) ; break ;
    }
}

/// @brief Flatten encoded load/store signed immediate pre/post index instruction.
void to_word_ls_simm(word32_t *dest, imm imm9, enc_ls_imm_idx idx) {
    set_bits_at(dest, 10, idx) ;
    set_bits_at(dest, 12, imm9 & 0x1ff) ;
}

/// @brief Flatten encoded load/store unsigned immediate offset instruction.
void to_word_ls_uimm(word32_t *dest, imm imm12) {
    set_bit_at(dest, 24, 1) ;
    set_bits_at(dest, 10, imm12 & 0xfff) ;
}

/// @brief Flatten encoded load/store immediate instruction.
void to_word_ls_imm(word32_t *dest, enc_ls_imm i) {
    set_bits_at(dest, 31, 1) ;
    set_bits_at(dest, 27, 0b111) ;
    set_bit_at(dest, 22, i.is_ldr) ;
    set_bits_at(dest, 5, i.xn) ;
    if (i.is_unsigned) to_word_ls_uimm(dest, i.imm12) ;
    else to_word_ls_simm(dest, i.imm9, i.idx) ;
    // set_bits_at(dest, 12, i.imm12) ;
}

/// @brief Flatten encoded load/store register instruction.
void to_word_ls_reg(word32_t *dest, enc_ls_reg i) {
    set_bit_at(dest, 31, 1) ;
    set_bits_at(dest, 27, 0b111) ;
    set_bit_at(dest, 22, i.is_ldr) ;
    set_bit_at(dest, 21, 1) ;
    set_bits_at(dest, 16, i.rm) ;
    set_bits_at(dest, 13, i.extend_tp) ;
    set_bit_at(dest, 12, i.shift) ;
    set_bit_at(dest, 11, 1) ;
    set_bits_at(dest, 5, i.xn) ;
}

/// @brief Flatten encoded load literal instruction.
void to_word_ld_lit(word32_t *dest, enc_ld_lit i) {
    set_bit_at(dest, 31, 0) ;
    set_bits_at(dest, 27, 0b11) ;
    set_bits_at(dest, 5, i.imm19 & 0x7FFFF) ;
}

/// @brief Flatten encoded structured LDR/STR instruction.
void to_word_ls(word32_t *dest, enc_ls i) {
    set_bit_at(dest, 30, i.sf) ;
    set_bit_at(dest, 27, 1) ;
    set_bit_at(dest, 25, 0) ;
    set_bits_at(dest, 0, i.xt) ;

    switch(i.tp) {
    case E_LS_IMM: to_word_ls_imm(dest, i.imm) ; break ;
    case E_LS_REG: to_word_ls_reg(dest, i.reg) ; break ;
    case E_LD_LIT: to_word_ld_lit(dest, i.lit) ; break ;
    }
}

/**
 * @brief Flattens structured encoding `i` to a 32-bit word.
 * 
 * @param i 
 * @return word32_t 
 */
word32_t to_word_enc(enc_instr i) {
    word32_t res = 0 ;
    word32_t *dest = &res ;
    switch (i.tp) 
    {
    case E_DP_IMM: to_word_dp_imm(dest, i.dp_imm) ; break ;
    case E_DP_REG: to_word_dp_reg(dest, i.dp_reg) ; break ;
    case E_INT_DIRECTIVE: res = i.int_directive ; break ;
    case E_NOP: res = NOP_CODE ; break ;
    case E_LS: to_word_ls(dest, i.ls) ; break ;
    case E_BRANCH: to_word_branch(dest, i.branch) ; break ;
    }
    return res ;
}
