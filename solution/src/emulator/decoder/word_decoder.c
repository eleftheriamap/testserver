#include <setjmp.h>

#include "emulator/decoder/word_decoder.h"
#include "utils/bits.h"
#include "utils/log.h"

jmp_buf decw_error ;

#define decw_error_handler(format, ...) log_error_handler(decw_error, 1, format, __VA_ARGS__)

/************************* decwode *************************/

/// @brief Decode word to an add w/ immediate instruction
void decw_add_imm(enc_add_imm *dest, word32_t c) {
    dest->is_subtract = bit_at(c, 30) ;
    dest->set_cond_flags = bit_at(c, 29) ;
    dest->shift_imm = bit_at(c, 22) ;
    dest->imm12 = bits_at(c, 21, 10) ;
    dest->xn = bits_at(c, 9, 5) ;
}

/// @brief Decode word to a mov instruction 
void decw_mov(enc_mov *dest, word32_t c) {
    dest->xd = bits_at(c, 4, 0) ;
    dest->imm16 = bits_at(c, 20, 5) ;
    dest->op_tp = bits_at(c, 30, 29) ;
    dest->shift = bits_at(c, 22, 21) ;
}

/// @brief Decode word to a data processing (immediate) instruction
void decw_dp_imm(enc_dp_imm *dest, word32_t c) {
    dest->sf = bit_at(c, 31) ;
    dest->xd = bits_at(c, 4, 0) ;

    bits_t op0 = bits_at(c, 25, 23) ;
    switch (op0) 
    {
    case 0b010: dest->tp = DP_ADD ; decw_add_imm(&dest->add_imm, c) ; break;
    case 0b100: not_implemented_error("decw_log_imm") ; break ;
    case 0b101: dest->tp = DP_MOV ; decw_mov(&dest->mov, c) ; break ;
    default: decw_error_handler("Not valid dp_imm op0: %d", op0) ;
    }
}

/// @brief Decode add w/ register instruction 
void decw_add_reg(enc_add_reg *dest, word32_t c) {
    dest->is_subtract = bit_at(c, 30) ;
    dest->set_cond_flags = bit_at(c, 29) ;
    dest->shift_type = bits_at(c, 23, 22) ;
    dest->shift_amount = bits_at(c, 15, 10) ; 
}

/// @brief Decode word to logical w/ register instruction
void decw_log_reg(enc_log_reg *dest, word32_t c) {
    // dest->negate = bit_at(c, 30) ;
    // not_implemented_error("decw_log_reg") ;
    dest->opc = bits_at(c, 30, 29) ;
    dest->negate = bit_at(c, 21) ;
    dest->shift_amount = bits_at(c, 15, 10) ;
    dest->shift_type = bits_at(c, 23, 22) ;
}

/// @brief Decode word to a multiplication instruction
void decw_mul(enc_mul *dest, word32_t c) {
    dest->xa = bits_at(c, 14, 10) ;
    dest->is_negate = bit_at(c, 15) ;
}

/// @brief Decode word to a data processing (register) instruction 
void decw_dp_reg(enc_dp_reg *dest, word32_t c) {
    dest->sf = bit_at(c, 31) ;
    dest->xm = bits_at(c, 20, 16) ;
    dest->xn = bits_at(c, 9, 5) ;
    dest->xd = bits_at(c, 4, 0) ;

    uint32_t op2 = bits_at(c, 24, 21) ;
    bool op1 = bit_at(c, 28) ;

    if (op1 && bit_at(op2, 3)) {
        dest->tp = DP_MUL ;
        decw_mul(&dest->mul, c) ;
    }
    else if (!op1 && bit_at_is(op2, 3, 0)) {
        dest->tp = DP_LOG ;
        decw_log_reg(&dest->log_reg, c) ;
        return ;
    } else if ( bit_at_is(op2, 0, 0) && bit_at_is(op2, 3, 1)) {
        dest->tp = DP_ADD ;
        decw_add_reg(&dest->add_reg, c) ;
        return ;
    } else if (op2 == 0b1000) {
        not_implemented_error("decw_multiply") ;
    } else {
        decw_error_handler("Not valid dp_reg op2: %d", op2) ;
    }
}

/// @brief Decode word to conditional branch instruction.
void decw_b_cond(enc_b_cond *dest, word32_t c) {
    dest->cond = bits_at(c, 3, 0) ;
    dest->imm19 = bits_at(c, 23, 5) ;
}

/// @brief Decode word to branch immediate offset instruction.
void decw_b_imm(enc_b_imm *dest, word32_t c) {
    dest->imm26 = bits_at(c, 26, 0) ;
}

/// @brief Decode word to branch register offset instruction.
void decw_b_reg(enc_b_reg *dest, word32_t c) {
    dest->xn = bits_at(c, 9, 5) ;
}

/// @brief Decode word to a branch instruction. 
void decw_branch(enc_branch *dest, word32_t c) {
    uint32_t op0 = bits_at(c, 31, 29) ;

    if (op0 == 0b010) {
        dest->tp = E_B_COND ;
        decw_b_cond(&dest->cond, c) ;
    } else if (bits_at(op0, 1, 0) == 0b00) {
        dest->tp = E_B_IMM ;
        decw_b_imm(&dest->imm, c) ;
    } else if (op0 == 0b110) {
        dest->tp = E_B_REG ;
        decw_b_reg(&dest->reg, c) ;
    } else {
        decw_error_handler("Not valid branch op0: %d", op0) ;
    }
}

void decw_ld_lit(enc_ld_lit *dest, word32_t c) {
    dest->imm19 = bits_at(c, 23, 5) ;
}

void decw_ls_reg(enc_ls_reg *dest, word32_t c) {
    dest->extend_tp = bits_at(c, 15, 13) ;
    dest->shift = bit_at(c, 12) ;
    dest->xn = bits_at(c, 9, 5) ;
    dest->rm = bits_at(c, 20, 16) ;
    dest->is_ldr = bit_at(c, 22) ;
}

void decw_ls_simm(enc_ls_imm *dest, word32_t c) {
    dest->is_unsigned = false ;
    dest->imm9 = bits_at(c, 20, 12) ;
    dest->xn = bits_at(c, 9, 5) ;
    dest->is_ldr = bit_at(c, 22) ;
    dest->idx = bits_at(c, 11, 10) ;
}

void decw_ls_uimm(enc_ls_imm *dest, word32_t c) {
    dest->is_unsigned = true ;
    dest->imm12 = bits_at(c, 21, 10) ;
    dest->xn = bits_at(c, 9, 5) ;
    dest->is_ldr = bit_at(c, 22) ;
}

/// @brief Decode word to structured load/store encoding. 
void decw_ls(enc_ls *dest, word32_t c) {
    dest->sf = bit_at(c, 30) ;
    uint64_t op0 = bits_at(c, 29, 28) ;
    uint64_t op2 = bits_at(c, 24, 23) ;
    bool op3 = bit_at(c, 21) ;
    uint64_t op4 = bits_at(c, 11, 10) ;

    dest->xt = bits_at(c, 4, 0) ;

    if (op0 == 1 && (op2 >> 1) == 0) {
        dest->tp = E_LD_LIT ;
        decw_ld_lit(&dest->lit, c) ;
    } else if (op0 == 0b11 && op4 == 0b10 && op3 && (op2 >> 1) == 0) {
        dest->tp = E_LS_REG ;
        decw_ls_reg(&dest->reg, c) ;
    } else if (op0 == 0b11 && (op2 >> 1) == 1) {
        dest->tp = E_LS_IMM ;
        decw_ls_uimm(&dest->imm, c) ;
    } else if (op0 == 0b11 && (op2 != 0b10) && !op3) {
        dest->tp = E_LS_IMM ;
        decw_ls_simm(&dest->imm, c) ;
    } else {
        decw_error_handler("Not valid ls op0: %d", op0) ;
    }

}


/// @brief Decode word to structured instruction encoding.
/// @param dest Where to store the decoded instruction.
/// @param c The word to decode.
/// @return Boolean indicating success or failure of decoding.
bool dec_word(enc_instr *dest, word32_t c) {
    int ret = setjmp(decw_error) ;
    if (ret != 0) {
        return false ;
    }

    if (c == NOP_CODE) {
        dest->tp = E_NOP ;
        return true ;
    }

    uint32_t op0 = ((0xF << 25) & c) >> 25;

    if (bits_at(op0, 4, 1) == 0b100) {
        dest->tp = E_DP_IMM ;
        decw_dp_imm(&dest->dp_imm, c) ;
    } else if ((op0 & 0b0111) == 0b101) {
        dest->tp = E_DP_REG ;
        decw_dp_reg(&dest->dp_reg, c) ;
    } else if (bits_at(op0, 4, 1) == 0b101) {
        dest->tp = E_BRANCH ;
        decw_branch(&dest->branch, c) ;
    } else if (!bit_at(op0, 0) && bit_at(op0, 2)) {
        dest->tp = E_LS ;
        decw_ls(&dest->ls, c) ;
    } else {
        decw_error_handler("instr not decodable: %x", c) ;
    }

    return true ;
}
