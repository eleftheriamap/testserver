#include <setjmp.h>

#include "emulator/decoder/enc_decoder.h"
#include "utils/log.h"
#include "utils/bits.h"

#define DEC_CASE(cond, enum_tp, sub_tp) \
    case cond: dest->tp = enum_tp ; dec_ ## sub_tp (dest, i. ## sub_tp) ; break

#define TO_ADD_ENUM(i) (DP_ADD + ((i.is_subtract << 1) + i.set_cond_flags))

static jmp_buf dece_error ;

#define dece_error_handler(args...) log_error_handler(dece_error, 1, args)


static reg_t dec_reg(uint64_t rn, reg_e r31, bool is_extended) {
    if (rn == 31) return (reg_t) {.r = r31, .extended = is_extended} ;
    return (reg_t) {.r = rn, .extended = is_extended} ;
}

/// @brief Decodes an encoded add instruction with immediate operands.
void dec_add_imm(instr_dp *dest, enc_add_imm i, bool is_extended) {
    dest->op_type = TO_ADD_ENUM(i) ;
    dest->op2.imm_sh.imm = i.imm12 ;
    dest->op2.imm_sh.sh.tp = LSL ;
    dest->op2.imm_sh.sh.amount = i.shift_imm ? 12 : 0 ;
    dest->rn = dec_reg(i.xn, RZR, is_extended) ; // ! TODO check if sets flags or not
}

/// @brief Decodes an encoded mov instruction.
void dec_mov(instr_dp *dest, enc_mov i, bool is_extended) {
    dest->op2.imm_sh.imm = i.imm16 ;
    dest->op2.imm_sh.sh.tp = LSL ;
    dest->op2.imm_sh.sh.amount = i.shift * 16 ;

    switch(i.op_tp) {
        case E_MOVN: dest->op_type = OP_MOVN ; break ;
        case E_MOVZ: dest->op_type = OP_MOVZ ; break ;
        case E_MOVK: dest->op_type = OP_MOVK ; break ;
    }
}

/// @brief Decodes an encoded data processing instruction with immediate operands.
void dec_dp_imm(instr_dp *dest, enc_dp_imm i) {
    bool is_extended = i.sf ;
    dest->rd = dec_reg(i.xd, RZR, is_extended) ; // TODO: check if sets flags or not, because then is SP for DP_ADD
    dest->op2.type = OP2_IMM_SH ;

    switch (i.tp) {
    case DP_ADD:
        dec_add_imm(dest, i.add_imm, is_extended) ;
        break ;
    case DP_MOV:
        dec_mov(dest, i.mov, is_extended) ;
        break ;
    default:
        dece_error_handler("Not valid dp_imm tp: %d", i.tp) ;
    }
}

/// @brief Decodes an encoded add instruction with register operands. 
void dec_add_reg(instr_dp *dest, enc_add_reg i) {
    dest->op_type = TO_ADD_ENUM(i) ;
    dest->op2.reg_sh.sh.amount = i.shift_amount ;
    dest->op2.reg_sh.sh.tp = i.shift_type ;
}

/// @brief Decodes an encoded logical instruction with register operands. 
void dec_log_reg(instr_dp *dest, enc_log_reg i) {
    dest->op_type = (i.opc << 1) + i.negate + OP_AND ;
    dest->op2.reg_sh.sh.amount = i.shift_amount ;
    dest->op2.reg_sh.sh.tp = i.shift_type ;
}

/// @brief Decodes an encoded multiply instruction.
void dec_mul(instr_dp *dest, enc_dp_reg i, bool is_extended) {
    dest->op_type = i.mul.is_negate ? OP_MSUB : OP_MADD ;
    dest->op2.type = OP2_MUL ;
    dest->op2.mul.ra = dec_reg(i.mul.xa, RZR, is_extended) ;
    dest->op2.mul.rm = dec_reg(i.xm, RZR, is_extended) ;
}

/// @brief Decodes an encoded data processing instruction with register operands. 
void dec_dp_reg(instr_dp *dest, enc_dp_reg i) {
    bool is_extended = i.sf ;
    dest->rd = dec_reg(i.xd, RZR, is_extended) ;
    dest->rn = dec_reg(i.xn, RZR, is_extended) ;
    dest->op2.type = OP2_REG_SH ;
    dest->op2.reg_sh.rm = dec_reg(i.xm, RZR, is_extended) ;

    switch (i.tp) {
    case DP_ADD: dec_add_reg(dest, i.add_reg) ; break ;
    case DP_LOG: dec_log_reg(dest, i.log_reg) ; break ;
    case DP_MUL: dec_mul(dest, i, is_extended) ; break ;
    default:
        dece_error_handler("dec_dp_reg") ;
    }
}

/// @brief Calculates the address given by an offset and the current PC.
/// @param offset The offset to be added to the current PC.
/// @param bit_size The size of the offset, for calculating how to sign extend it to 64-bits.
/// @param curr_pc The current PC.
/// @return The address calculated.
address_t calc_address(uint32_t offset, size_t bit_size, address_t curr_pc) {
    // Sign extend offset to 64 bits
    uint64_t u_offset = offset ;
    if (bit_at(offset, bit_size - 1)) {
        u_offset |= (0xFFFFFFFFFFFFFFFF << (bit_size)) ;
    }
    int64_t s_offset = (int64_t) u_offset ;
    return (address_t) curr_pc + (s_offset * 4) ;
}

/// @brief Decodes an encoded branch instruction into an instr_b AST Node.
void dec_branch(instr_b *dest, enc_branch i, address_t curr_pc) {
    switch (i.tp) {
    case E_B_IMM: 
        dest->tp = TP_B ;
        dest->address = calc_address(i.imm.imm26, 26, curr_pc) ;
        dest->label = "<decoded>" ;
        break ;
    case E_B_REG:
        dest->tp = TP_BR ;
        dest->rn = dec_reg(i.reg.xn, -1, true) ;
        break ;
    case E_B_COND:
        dest->tp = TP_BCond ;
        dest->cond = i.cond.cond ;
        dest->address = calc_address(i.cond.imm19, 19, curr_pc) ;
        dest->label = "decoded" ;
        break ;
    }
}

void dec_ls_imm(i_ls_imm *dest, enc_ls_imm i) {
    if (i.is_unsigned) {
        dest->imm = ((uint64_t) i.imm12) * 8;
        dest->idx_tp = IDX_U_OFFSET ;
    } else {
        dest->imm =  sign_extend(i.imm9, 9, 64) ;
        switch (i.idx) {
        case E_LS_IDX_POST: dest->idx_tp = IDX_POST ; break ;
        case E_LS_IDX_PRE: dest->idx_tp = IDX_PRE ; break ;
        }
    }
    dest->rn = dec_reg(i.xn, SP, true) ;
}

void dec_ls_reg(i_ls_reg *dest, enc_ls_reg i) {
    dest->rn = dec_reg(i.xn, SP, true) ;
    dest->rm = dec_reg(i.rm, RZR, true) ; // TODO not necessarily extended
    dest->extend.tp = i.extend_tp;
    dest->extend.amount = i.shift * 3 ;
}

void dec_ls_lit(i_ls_lit *dest, enc_ld_lit i, address_t curr_pc) {
    dest->lit = calc_address(i.imm19, 19, curr_pc) ; ;
    dest->label = "<decoded>" ;
}

/// @brief Decodes an encoded load/store instruction into an instr_ls AST Node. 
void dec_ls(instr_ls *dest, enc_ls i, address_t curr_pc) {
    bool is_extended = i.sf ;
    dest->rt = dec_reg(i.xt, -1, is_extended ) ;
    switch (i.tp) {
    case E_LS_IMM:
        dest->arg_tp = LS_IMM ;
        dest->op = i.imm.is_ldr ? OP_LDR : OP_STR ;
        dec_ls_imm(&dest->imm, i.imm) ;
        break ;
    case E_LS_REG:
        dest->arg_tp = LS_REG ;
        dest->op = i.reg.is_ldr ? OP_LDR : OP_STR ;
        dec_ls_reg(&dest->reg, i.reg) ;
        break ;
    case E_LD_LIT:
        dest->arg_tp = LS_LIT ;
        dest->op = OP_LDR ;
        dec_ls_lit(&dest->lit, i.lit, curr_pc) ;
        break ;
    }
}

/// @brief Decodes an encoded instruction into an instr_t AST Node.
/// @param dest Where to store the decoded instruction.
/// @param i The encoded instruction.
/// @param curr_pc Current program counter value.
/// @return Boolean indicating whether the decoding was successful.
bool decode_enc_instr(instr_t *dest, enc_instr i, address_t curr_pc) {
    int ret = setjmp(dece_error) ;
    if (ret != 0) {
        logln("^ in decode_enc_instr") ;
        return false ;
    }

    switch (i.tp) {
    case E_DP_IMM: 
        dest->tp = I_DP ; 
        dec_dp_imm (&dest->dp, i.dp_imm) ; 
        break ;
    case E_DP_REG: 
        dest->tp = I_DP ; 
        dec_dp_reg (&dest->dp, i.dp_reg) ; 
        break ;
    case E_INT_DIRECTIVE: 
        dest->tp = I_DIRECTIVE ;
        dest->int_directive = i.int_directive ; 
        break ;
    case E_BRANCH:
        dest->tp = I_B ;
        dec_branch(&dest->b, i.branch, curr_pc) ;
        break ;
    case E_LS:
        dest->tp = I_LS ;
        dec_ls(&dest->ls, i.ls, curr_pc) ;
        break ;
    case E_NOP:
        dest->tp = I_NOP ;
        break ;
    default: 
        dece_error_handler("Unrecognized instruction type: %d\n", i.tp) ;
    }
    return true ;
}