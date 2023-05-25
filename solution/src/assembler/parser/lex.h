/**
 * @file lex.h
 * @author David Davies
 * @brief Contains all tokens and lexing functions
 * 
 */

#ifndef __LEX_H
#define __LEX_H


/// @brief Operator name tokens
typedef enum {
    TOK_MOV,TOK_MOVN,TOK_MOVZ,TOK_MOVK,
    TOK_TST,
    // NOTE: decoding relies on this order of ADD, ... SUBS.
    TOK_ADD,TOK_SUB,TOK_ADDS,TOK_SUBS,
    TOK_CMP, TOK_CMN,
    // Log imm + Reg
    TOK_AND, TOK_ORR, TOK_EOR, TOK_ANDS,
    // Log reg
    TOK_BIC, TOK_ORN, TOK_EON, TOK_BICS,
    TOK_LDR,TOK_STR,
    TOK_B,TOK_BCond, TOK_BR,
    TOK_DotInt,
    TOK_NOP,
    // Multiplication
    TOK_MUL, TOK_MADD, TOK_MNEG, TOK_MSUB
} op_tok ;

typedef enum {
    // TOK_EQ,TOK_NE,TOK_GE,TOK_LT,TOK_GT,TOK_LE,TOK_AL,
    TOK_EQ = 0x0,
    TOK_NE = 0x1,
    TOK_GE = 0xa,
    TOK_LT = 0xb,
    TOK_GT = 0xc,
    TOK_LE = 0xd,
    TOK_AL = 0xe
} cond_tok ;
    // Shifts
typedef enum {
    TOK_LSL,TOK_LSR,TOK_ASR,TOK_ROR
} shift_tok ;

typedef enum {
    TOK_EXTEND_LSL = 0b011, 
    TOK_EXTEND_SXTX = 0b111,
}   extend_tok ;

#endif
