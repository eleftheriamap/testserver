/**
 * @file parse_table.h
 * @brief Tables for parsing opcodes, conditions and shifts
 * 
 */

#ifndef __PARSE_TABLE_H
#define __PARSE_TABLE_H

#include <stdint.h>

#include "common/ast.h"
#include "assembler/parser/lex.h"


typedef struct enum_binding {
    const char *key;
    uint64_t e;
}   enum_binding ;

typedef enum_binding cond_binding;
typedef enum_binding shift_binding;

typedef struct prsr {
    char *tok ;
    char **rest ;
}   prsr ;

typedef void (*p_func_t)(instr_t *, prsr *, void *)  ;

typedef size_t any_enum ;

typedef struct op_binding {
    const char *key ;
    op_tok op ;
    any_enum ast_op ;
    p_func_t p_func ;
}   op_binding ;

typedef struct label_binding {
    const char *key ;
    uint64_t addr ;
}   label_binding ;

#define ENUM_TABLE_ADD(name, tok_tp) \
    int add_ ## name ## _bind(const char *key, tok_tp val)
#define ENUM_TABLE_GET(name, tok_tp) \
    tok_tp get_ ## name ## _bind(const char *key)
#define ENUM_TABLE_SEARCH(name, tok_tp, res_ptr) \
    bool search_ ## name ## _bind(const char *key, any_enum *res_ptr)
#define ENUM_TABLE_HEADERS(name, tok_tp) \
    ENUM_TABLE_ADD(name, tok_tp) ;       \
    ENUM_TABLE_GET(name, tok_tp) ;       \
    ENUM_TABLE_SEARCH(name, tok_tp, res)

ENUM_TABLE_HEADERS(cond, cond_tok) ;
// ENUM_TABLE_HEADERS(reg, reg_t) ;
ENUM_TABLE_HEADERS(shift, shift_tok) ;
ENUM_TABLE_HEADERS(extend, extend_tok) ;

int add_label(const char *key, uint64_t addr) ;
label_binding *get_label(const char *key) ;

int add_op_bind(const char *key, op_tok op, any_enum ast_op, p_func_t p_func) ;
op_binding *get_op_bind(const char *key) ;

void init_tables() ;
void free_tables() ;

#endif
