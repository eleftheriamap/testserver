#include <stdint.h>

#include "assembler/encoder.h"
#include "utils/bits.h"

static address_t curr_address = 0;

/*********************************************************************************************************************/
// Atoms/Utils

uint32_t enc_reg(reg_t reg) {
    if (reg.r == RZR || reg.r == SP || reg.r == PC) return 0x1f;
    return reg.r;
}

uint32_t enc_shift(shift_tp shift) {
    return shift;
}

uint32_t enc_imm(imm i) {
    return i;
}

uint32_t enc_bool(bool b) {
    return b ? 1 : 0 ;
}


uint64_t calc_offset (size_t size, address_t from, address_t to) {
    int64_t offset = ((int64_t) ( to - from)) / 4 ;
    ASSERT_M(offset < (1 << size) && offset >= -(1 << size), "Offset too large: %ld", offset) ;
    bool top_bit = 1 ? offset < 0 :  0 ;
    uint64_t val2 = 
            (top_bit << (size - 1)) 
        |   (offset & BIT_MASK(size - 1)) ;
    return val2 ;
}



/*********************************************************************************************************************/
// Data Processing Immediate

#define add_op_from_code(c, is_subtract, set_cond_flags) switch (c) {   \
    case OP_ADD:  is_subtract = 0; set_cond_flags = 0; break;           \
    case OP_ADDS: is_subtract = 0; set_cond_flags = 1; break;           \
    case OP_SUB:  is_subtract = 1; set_cond_flags = 0; break;           \
    case OP_SUBS: is_subtract = 1; set_cond_flags = 1; break;           \
    default:                                                            \
        log_error("Not an addition instruction: %u", c);                \
    }


/// @brief Encode an immediate addition instruction 
void encode_add_imm(enc_add_imm *dest, instr_dp i) {
    add_op_from_code(i.op_type, dest->is_subtract, dest->set_cond_flags) ;
    
    dest->xn = enc_reg(i.rn) ;
    dest->imm12 = i.op2.imm_sh.imm ;
    dest->shift_imm = i.op2.imm_sh.sh.amount;
}

void encode_mov_imm(enc_mov *dest, instr_dp i) {
    dest->imm16 = i.op2.imm_sh.imm ;
    dest->shift = i.op2.imm_sh.sh.amount / 16 ;
    dest->xd = enc_reg(i.rd) ;
    
    switch (i.op_type) {
    case OP_MOVN: dest->op_tp = E_MOVN ; break ;
    case OP_MOVZ: dest->op_tp = E_MOVZ ; break ;
    case OP_MOVK: dest->op_tp = E_MOVK ; break ;
    default: log_error("Not a mov instruction: %u", i.op_type) ;
    }
}


/*********************************************************************************************************************/
// Data Processing Register

/// @brief Encode a register addition instruction
void encode_add_reg(enc_add_reg *dest, instr_dp i) {
    add_op_from_code(i.op_type, dest->is_subtract, dest->set_cond_flags) ;
    
    dest->shift_amount = i.op2.reg_sh.sh.amount ;
    dest->shift_type = i.op2.reg_sh.sh.tp ;

}

/// @brief Encode a register logical instruction
void encode_log_reg(enc_log_reg *dest, instr_dp i) {
    dest->shift_amount = i.op2.reg_sh.sh.amount ;
    dest->shift_type = i.op2.reg_sh.sh.tp ;

    dest->opc = (i.op_type - OP_AND) >> 1 ;
    dest->negate = (i.op_type - OP_AND) & 1 ;
}


/*********************************************************************************************************************/
// Multiply

void encode_mul(enc_dp_reg *dest, instr_dp i) {
    dest->tp = DP_MUL ;
    dest->sf = i.rd.extended ;

    dest->xd = enc_reg(i.rd) ;
    dest->xn = enc_reg(i.rn) ;
    dest->xm = enc_reg(i.op2.mul.rm) ;
    dest->mul.xa = enc_reg(i.op2.mul.ra) ;
    dest->mul.is_negate = i.op_type == OP_MSUB ;
}

/*********************************************************************************************************************/
// Data Processing Cases

/**
 * @brief Abstract encoding of a data processing instruction with
 * immediate operand.
 * 
 * @param dest: destination of encoding.
 * @param i: instruction to be encoded.
 */
void encode_dp_imm(enc_dp_imm *dest, instr_dp i) {
    dest->sf = i.rd.extended ;
    dest->xd = enc_reg(i.rd) ;
    switch (i.op_type)
    {
    case ADD_CASES:
        dest->tp = DP_ADD ; 
        encode_add_imm (&dest->add_imm, i) ; break ;
    case MOV_CASES:
        dest->tp = DP_MOV ;
        encode_mov_imm (&dest->mov, i) ; break ;
    case LOG_CASES:
        log_exit_failure("Cannot encode logical instruction with immediate operand: %s", show_dp_op(i.op_type)) ;
    default: 
        log_exit_failure("Error encoding DP Immediate: unrecognised op_type: %u", i.op_type) ;
    }
}

/**
 * @brief Abstract encoding of a data processing instruction with register 
 * operand.
 * 
 * @param dest: destination of encoding.
 * @param i: instruction to be encoded.
 */
void encode_dp_reg(enc_dp_reg *dest, instr_dp i) {
    dest->sf = i.rd.extended ;
    dest->xn = enc_reg(i.rn) ;
    dest->xm = enc_reg(i.op2.reg_sh.rm) ;
    dest->xd = enc_reg(i.rd) ;

    switch (i.op_type)
    {
    case ADD_CASES: 
        dest->tp = DP_ADD;
        encode_add_reg(&dest->add_reg, i) ; break ;
    
    case LOG_CASES: 
        dest->tp = DP_LOG;
        encode_log_reg(&dest->log_reg, i) ; break ;
    default: 
    log_exit_failure("Error: Encoding DP Reg, Unrecognised op_type: %u", i.op_type) ;
    }
}


/*********************************************************************************************************************/
// Branching

void encode_b_cond(enc_b_cond *dest, instr_b i, address_t addr) {
    dest->cond = i.cond ;
    dest->imm19 = calc_offset(19, addr , i.address) ;
}

void encode_b_reg(enc_b_reg *dest, instr_b i) {
    dest->xn = enc_reg(i.rn) ;
}

void encode_b_imm(enc_b_imm *dest, instr_b i, address_t addr) {
    dest->imm26 = calc_offset(26, addr , i.address) ;
}

/*********************************************************************************************************************/
// Load/Store

void encode_ld_lit(enc_ld_lit *dest, i_ls_lit lit) {
    // ! TODO check this offset +4, it is only valid for 64 bit
    dest->imm19 = calc_offset(19, curr_address, lit.lit) ;
}

void encode_ls_imm(enc_ls_imm *dest, i_ls_imm i, bool is_ldr, bool is_extended) {
    dest->is_ldr = is_ldr ;
    dest->xn = enc_reg(i.rn) ;
    if (i.idx_tp == IDX_U_OFFSET) {
        dest->is_unsigned = true ;
        dest->imm12 = i.imm / (is_extended ? 8 : 4) ;
    } else {
        dest->is_unsigned = false ;
        dest->imm9 = i.imm ;
        dest->idx = i.idx_tp == IDX_PRE ? E_LS_IDX_PRE : E_LS_IDX_POST ;
    }
}

void encode_ls_reg(enc_ls_reg *dest, i_ls_reg i, bool is_ldr) {
    dest->is_ldr = is_ldr ;
    dest->xn = enc_reg(i.rn) ;
    dest->rm = enc_reg(i.rm) ;
    dest->shift = i.extend.amount != 0 ;
    dest->extend_tp = i.extend.tp ;
}

/*********************************************************************************************************************/
// Instruction ADT Cases

/**
 * @brief Places abstract encoding of abstract data processing instruction 
 * into `dest`.
 * 
 * @param dest: Destination of encoding 
 * @param i: The instruction to be encoded.
 */
void encode_dp(enc_instr *dest, instr_dp i) {
    switch(i.op2.type) {
    case OP2_IMM_SH: dest->tp = E_DP_IMM ; 
        encode_dp_imm(&dest->dp_imm, i) ; break ;
    case OP2_REG_SH: dest->tp = E_DP_REG ;
        encode_dp_reg(&dest->dp_reg, i) ; break ;
    case OP2_MUL: 
        dest->tp = E_DP_REG ;
        encode_mul(&dest->dp_reg, i) ; break ;

    default: log_exit_failure("enc_dp: bad op2 type") ;
    }
}

static inline
void encode_int_directive(enc_instr *dest, uint32_t dir) {
    dest->tp = E_INT_DIRECTIVE ;
    dest->int_directive = dir ;
}

static inline
void encode_nop(enc_instr *dest) {
    dest->tp = E_NOP ;
}

void encode_ls(enc_ls *dest, instr_ls i) {
    dest->sf = i.rt.extended ;
    dest->tp = i.arg_tp ;
    dest->xt = enc_reg(i.rt) ;
    switch (i.arg_tp) {
    case E_LS_IMM: encode_ls_imm(&dest->imm, i.imm, i.op == OP_LDR, dest->sf) ; break ;
    case E_LS_REG: encode_ls_reg(&dest->reg, i.reg, i.op == OP_LDR) ; break ;
    case E_LD_LIT: encode_ld_lit(&dest->lit, i.lit) ; break ;
    }
}


void encode_branch(enc_branch *dest, instr_b i, address_t addr) {
    dest->tp = E_BRANCH ;
    switch (i.tp) {
    case TP_B:
        dest->tp = E_B_IMM ;
        encode_b_imm(&dest->imm, i, addr) ; break ;
    case TP_BCond:
        dest->tp = E_B_COND ;
        encode_b_cond(&dest->cond, i, addr) ; break ;
    case TP_BR:
        dest->tp = E_B_REG ;
        encode_b_reg(&dest->reg, i) ; break ;
    }
}

/*********************************************************************************************************************/
// Instruction ADT

/**
 * @brief Translates the abstract instruction `i` to a representation
 * of it corresponding assembly encoding.
 * 
 * @param i: The instruction to be encoded
 * @return enc_instr: The structured encoding of `i`.
 */
enc_instr encode_instr(instr_t i) {
    enc_instr e;
    curr_address = i.address ;
    char *i_str = show_instr(i) ;
    loglvl(LOG_3, "encoding AST node: %s\n", i_str) ;
    free(i_str) ;
    switch(i.tp) {
    case I_DP: encode_dp(&e, i.dp) ; break ;
    case I_DIRECTIVE: encode_int_directive(&e, i.int_directive) ; break ;
    case I_NOP: e.tp = E_NOP ; encode_nop(&e) ; break ;
    case I_LS: e.tp = E_LS ; encode_ls(&e.ls, i.ls) ; break ;
    case I_B:
        e.tp = E_BRANCH ;
        encode_branch(&e.branch, i.b, i.address) ; break ;
    // default: log_exit_failure("encode_instr: Instr type not recognised: %u", i.tp) ;
    }

    return e ;
}