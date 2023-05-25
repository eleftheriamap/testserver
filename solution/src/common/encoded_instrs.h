#ifndef __enc_instr_H
#define __enc_instr_H

#include <stdlib.h>
#include "wrapper/io.h"

#include "utils/log.h"
#include "common/ast.h"

typedef uint32_t code_word ;
typedef uint32_t word32_t ;

/*****************************************************************************/
// Encoded Data Processing with shifted immediate operands

typedef struct {
    bool is_subtract :1 ;
    bool set_cond_flags :1 ;
    bool shift_imm :1 ;
    unsigned int imm12: 12 ;
    unsigned int xn :5 ;
}   enc_add_imm ;

typedef enum {
    E_MOVN = 0b00,
    E_MOVZ = 0b10,
    E_MOVK = 0b11,
}   enc_mov_type ;

typedef struct {
    unsigned int xd :5 ;
    unsigned int imm16 :16 ;
    enc_mov_type op_tp :2 ;
    unsigned int shift :2 ;
}   enc_mov ;

/*****************************************************************************/
// Encoded Data Processing with shifted register operands

typedef struct {
    bool is_subtract :1 ;
    bool set_cond_flags :1 ;
    unsigned int shift_type :2 ;
    unsigned int shift_amount: 6 ;
}   enc_add_reg ;

typedef struct {
    unsigned int opc :4 ;
    bool negate :1 ;
    unsigned int shift_type :2 ;
    unsigned int shift_amount: 6 ;
}   enc_log_reg ;

/*****************************************************************************/
// Encoded Data Processing with three operand registers

typedef struct {
    bool is_negate :1 ;
    unsigned int xa :5 ;
}   enc_mul ;


/*****************************************************************************/
// Encoded Branch

typedef struct {
    unsigned int xn :5 ;
}   enc_b_reg ;

typedef struct {
    unsigned int cond :4 ;
    unsigned int imm19 :19 ;
}   enc_b_cond ;

typedef struct {
    unsigned int imm26 :26 ;
}   enc_b_imm ;


/*****************************************************************************/
// Load/Store

typedef enum enc_ls_tp {
    E_LS_IMM, E_LS_REG, E_LD_LIT
}   enc_ls_tp ;

typedef enum enc_ls_imm_idx {
    E_LS_IDX_POST = 0b01,
    E_LS_IDX_PRE = 0b11,
}   enc_ls_imm_idx ;

typedef struct enc_ls_imm {
    bool is_ldr :1 ; // bit 22
    bool is_unsigned :1 ; // bit 24
    union {
        struct { // Unsigned offset
            unsigned int imm12 :12 ;
        }   ;
        struct { // Signed offset
            signed int imm9 :9 ;
            enc_ls_imm_idx idx :2 ;
        }   ;
    } ;

    unsigned int xn :5 ;
}   enc_ls_imm ;

typedef struct enc_ls_reg {
    bool is_ldr :1 ; // Bit 22
    bool shift :1 ;
    ls_reg_extend_tp extend_tp :3 ;
    unsigned int xn :5 ;
    unsigned int rm :5 ;
}   enc_ls_reg ;

typedef struct enc_ld_lit {
    signed int imm19 :19 ;
}   enc_ld_lit ;

/*****************************************************************************/
// ADT Cases

typedef enum {
    DP_ADD,
    DP_LOG,
    DP_MOV,
    DP_MUL
}   enc_dp_type ;

typedef enum {
    E_DP_IMM, E_DP_REG, E_INT_DIRECTIVE, E_NOP, E_BRANCH, E_LS
}   enc_type ;

/**
 * @brief Encoded DP instruction with shifted immediate argument
 * 
 */
typedef struct {
    enc_dp_type tp ;
    bool sf :1 ;
    unsigned int xd :5 ;
    union {
        enc_add_imm add_imm ;
        enc_mov mov ;
    } ;
}   enc_dp_imm ;

/**
 * @brief Encoded DP instruction with (shifted) register argument
 * 
 */
typedef struct {
    enc_dp_type tp ;
    bool sf :1 ;
    unsigned int xm :5 ;
    unsigned int xn :5 ;
    unsigned int xd :5 ;

    union {
        enc_add_reg add_reg ;
        enc_log_reg log_reg ;
        enc_mul mul ;
    } ;
}   enc_dp_reg ;

typedef enum {
    E_B_IMM, E_B_REG, E_B_COND
}   enc_b_type ;


/**
 * @brief Encoded branch instruction
 * 
 */
typedef struct {
    enc_b_type tp ;
    union {
        enc_b_imm imm ;
        enc_b_reg reg ;
        enc_b_cond cond ;
    } ;
}   enc_branch ;


typedef struct enc_ls {
    bool sf :1 ;
    enc_ls_tp tp ;
    union {
        enc_ls_imm imm ;
        enc_ls_reg reg ;
        enc_ld_lit lit ;
    }   ;
    unsigned int xt :5 ;
}   enc_ls ;

typedef uint32_t enc_int ;

/*****************************************************************************/
// Encoded Instruction ADT

typedef struct {
    enc_type tp ;
    union {
        enc_dp_imm dp_imm ;
        enc_dp_reg dp_reg ;
        enc_int int_directive ;
        enc_branch branch ;
        enc_ls ls ;
    } ;
}   enc_instr ;

#endif