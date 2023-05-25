#ifndef __AST_H
#define __AST_H

/**
 * @file ast.h
 * @brief Defines abstract syntax nodes for ARM64 instructions
 * 
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/// @brief A 64-bit ARM register: R0-30, SP, RZR or PC
typedef enum {
    R0,R1,R2,R3,R4,R5,
    R6,R7,R8,R9,R10,R11,
    R12,R13,R14,R15,R16,
    R17,R18,R19,R20,R21,
    R22,R23,R24,R25,R26,
    R27,R28,R29,R30,
    RZR,
    SP,
    PC
} reg_e ;


typedef struct reg_t {
    reg_e r ;
    bool extended ;
}   reg_t ;

extern const reg_t NULL_REG ;
bool is_null_reg(reg_t r) ;

/// @brief The data processing operation
typedef enum {
    OP_ADD, OP_ADDS, OP_SUB, OP_SUBS,
    // Order is important here
    OP_AND, OP_BIC, OP_ORR, OP_ORN, OP_EOR, OP_EON, OP_ANDS, OP_BICS,
    // Moves
    OP_MOVN, OP_MOVZ, OP_MOVK,
    // Multiplication
    OP_MADD, OP_MSUB
}   dp_op ;

bool is_mov(dp_op) ;

#define ADD_CASES OP_ADD: case OP_ADDS: case OP_SUB: case OP_SUBS
#define LOG_CASES OP_AND: case OP_ANDS: case OP_ORR: case OP_EOR: \
    case OP_ORN: case OP_EON: case OP_BIC: case OP_BICS
#define MOV_CASES OP_MOVN: case OP_MOVZ: case OP_MOVK

#define MUL_CASES OP_MADD: case OP_MSUB

#define OP_CMP OP_SUBS
#define OP_CMN OP_ADDS
#define OP_TST OP_ANDS

/// @brief The type of a shift: LSL, LSR, ASR
typedef enum {
    LSL, LSR, ASR, ROR
} shift_tp ;

/// @brief Contains a bit shift and how much to shift by
typedef struct {
    shift_tp tp ;
    uint32_t amount ;
} shift_arg ;

typedef uint32_t imm ;


/*****************************************************************************/
// Operand 2 of DP instructions


/// @brief The type of the 2nd operand of a data processing instruction.
typedef enum {
    OP2_IMM_SH, OP2_REG_SH, OP2_MUL
} op2_tp  ;

/// @brief A (shifted) immediate 2nd operand
typedef struct op2_imm {
    imm imm ;
    shift_arg sh ;
}   op2_imm ;

#define DEFAULT_SHIFT ((shift_arg) {.tp = LSL, .amount = 0 })

/// @brief A (shifted) register 2nd operand
typedef struct op2_reg {
    reg_t rm       ;
    shift_arg sh ;
} op2_reg ;

typedef struct op2_mul {
    reg_t rm ;
    reg_t ra ;
}   op2_mul ;

/**
 * @brief ADT for data processing 2nd operand.
 * 
 * @details ADT of 2nd operands of data processing instructions.
 * Case is determined by `type`.
 */
typedef struct op2_t {
    op2_tp type ;
    union {
        op2_reg reg_sh ;
        op2_imm imm_sh ;
        op2_mul mul ;
    } ;
}   op2_t ;

typedef enum cond_e {
    EQ = 0x0,
    NE = 0x1,
    GE = 0xa,
    LT = 0xb,
    GT = 0xc,
    LE = 0xd,
    AL = 0xe
} cond_e ;



/*****************************************************************************/
// Instruction ADT Cases

/// @brief The different cases of an assembly instruction/line
typedef enum instr_type {
    I_DP, I_B, I_LS, I_DIRECTIVE, I_NOP
}   instr_type ;

/**
 * @struct instr_dp 
 *  Abstract data processing instruction.
 *  This could have a shifted register or immediate operand in `op2`.
 * 
 * @var instr_dp::op_type
 *   erm
 */
typedef struct {
    dp_op op_type ;
    reg_t rd;
    reg_t rn ;
    op2_t op2 ;
}   instr_dp ;


typedef uint64_t address_t ;

typedef enum {
    TP_B,
    TP_BCond,
    TP_BR,
}   e_b_tp ;

typedef struct {
    e_b_tp tp ;
    union {
        reg_t rn ;
        struct {
            address_t address ;
            const char *label ; // Useful for debugging
            cond_e cond ;
        }   ;
    } ;
}   instr_b ;

typedef enum {
    OP_STR,
    OP_LDR,
}   e_ls_op ;

typedef enum {
    IDX_POST,
    IDX_PRE,
    IDX_U_OFFSET,
}   e_ls_idx ;

typedef struct {
    e_ls_idx idx_tp ;
    reg_t rn ;
    imm imm;
}   i_ls_imm ;

typedef enum ls_reg_extend_tp {
    // E_LS_EXTEND_UXTW = 0b010,
    E_LS_EXTEND_LSL  = 0b011,
    // E_LS_EXTEND_SXTW = 0b110,
    E_LS_EXTEND_SXTX = 0b111,
}   ls_reg_extend_tp ;

// typedef enum extend_tp {
//     EXT_UXTW, EXT_LSL, EXT_SXTW, EXT_SXTX
// }   extend_tp ;

typedef struct extend {
    ls_reg_extend_tp tp ;
    uint32_t amount ;
}   extend_t ;

typedef struct {
    reg_t rn ;
    reg_t rm ;
    extend_t extend ;
}   i_ls_reg ;

typedef enum {
    LS_IMM,
    LS_REG,
    LS_LIT,
}   e_ls_tp ;

typedef struct {
    imm lit ;
    const char *label ;
}   i_ls_lit ;

typedef struct {
    e_ls_op op ;
    e_ls_tp arg_tp ;
    reg_t rt ;
    union {
        i_ls_imm imm ;
        i_ls_reg reg ;
        i_ls_lit lit ;
    } ;
}   instr_ls ;



/*****************************************************************************/
// Instruction ADT

/**
 * Abstract instruction syntax
 * 
 *
 * @struct instr_t
 *      Abstract instruction syntax
 * 
 * @var instr_t::address
 * 
 * @var instr_t::tp
 * 
 * @var 
 */
typedef struct {
    address_t address ;
    /// @brief The type of the instruction, used to identify the case of the union.
    instr_type tp ;
    union {
        instr_dp dp ;
        uint32_t int_directive ;
        instr_b b ;
        instr_ls ls ;
    } ;
} instr_t ;

char *show_instr(instr_t instr) ;
char *show_reg(reg_t r) ;
const char *show_shift(shift_tp s) ;
const char *show_dp_op(dp_op op) ;

typedef struct block_t {
    char *label ;
    size_t row ;
    instr_t ** instrs ;
}   block_t ;

reg_t zero_reg(bool extended) ;

#endif
