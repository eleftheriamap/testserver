#include "wrapper/io.h"
#include <string.h>

#include "common/ast.h"
#include "common/ast/reg.h"
#include "utils/log.h"


bool is_mov(dp_op op) {
    return op == OP_MOVN || op == OP_MOVZ || op == OP_MOVK ;
}

void append_to_dest(char **dest, const char *format, ...) {
    va_list args ;
    va_start(args, format) ;
    int size_written = vsprintf(*dest, format, args) ;
    *dest += size_written ;
    if (size_written < 0) log_exit_failure("ast.c:append_to_dest: Error while writing to dest: %s; %s", *dest, format) ;
    va_end(args) ;
}

/// @brief The string representation of a shift 
const char *__str_shift(shift_tp s) {
    char *res ;
    switch (s) {
        case LSL: res = "lsl" ; break ;
        case LSR: res = "lsr" ; break ;
        case ASR: res = "asr" ; break ;
        case ROR: res = "ror" ; break ;
    }
    return res ;
}

/// @brief Concatenate the string representation of an immediate shifted operand to dest
void __catstr_op2_imm_sh(char **dest, op2_imm imm_sh) {
    append_to_dest(dest, "#0x%x", imm_sh.imm) ;

    if (imm_sh.sh.tp == LSL && imm_sh.sh.amount == 0) return ;
    append_to_dest(dest, ", %s #%d", __str_shift(imm_sh.sh.tp), imm_sh.sh.amount) ;
}

/// @brief Concatenate the string representation of a shifted register to dest
void __catstr_op2_reg_sh(char **dest, op2_reg reg_sh) {
    if (reg_sh.sh.tp == LSL && reg_sh.sh.amount == 0)  {
        append_to_dest(dest, "%s", __str_reg(reg_sh.rm)) ;
        return ;
    }
    append_to_dest(dest, "%s, %s #%d", __str_reg(reg_sh.rm), __str_shift(reg_sh.sh.tp), reg_sh.sh.amount) ;
}

/// @brief Concatenate the string representation of mul registers rm and ra to dest
void __catstr_op2_mul(char **dest, op2_mul mul) {
    append_to_dest(dest, "%s, %s", __str_reg(mul.rm), __str_reg(mul.ra)) ;
}

/// @brief Concatenate the string representation of a nop to dest
void __catstr_instr_nop(char **dest) {
    append_to_dest(dest, "nop") ;
}

/// @brief Concatenate the string representation of an int_directive to dest
void __catstr_instr_directive(char **dest, uint32_t int_directive) {
    append_to_dest(dest, ".word 0x%x", int_directive) ;
}

/// @brief Concatenate the string representation of an op2 to dest
void __catstr_op2(char **dest, op2_t op2) {
    switch (op2.type) {
        case OP2_IMM_SH: __catstr_op2_imm_sh(dest, op2.imm_sh) ; break ;
        case OP2_REG_SH: __catstr_op2_reg_sh(dest, op2.reg_sh) ; break ;
        case OP2_MUL:    __catstr_op2_mul(dest, op2.mul) ; break ;
        default:
        log_exit_failure("Unknown op2 type %u", op2.type) ;
    }
}

/// @brief The string representation of a dp operation 
const char *__str_dp_op(dp_op op) {
    switch (op) {
        case OP_ADD:  return "add" ;
        case OP_ADDS: return "adds" ;
        case OP_SUB:  return "sub" ;
        case OP_SUBS: return "subs" ;
        case OP_AND:  return "and" ;
        case OP_BIC:  return "bic" ;
        case OP_ORR:  return "orr" ;
        case OP_ORN:  return "orn" ;
        case OP_EOR:  return "eor" ;
        case OP_EON:  return "eon" ;
        case OP_ANDS: return "ands" ;
        case OP_BICS: return "bics" ;
        case OP_MOVN: return "movn" ;
        case OP_MOVZ: return "movz" ;
        case OP_MOVK: return "movk" ;
        case OP_MADD: return "madd" ;
        case OP_MSUB: return "msub" ;
    }
    log_exit_failure("Unknown dp_op %u", op) ;
}


/// @brief Concatenate the string representation of a dp instruction to dest
void __catstr_instr_dp(char **dest, instr_dp instr) {
    append_to_dest(dest, "%s %s, ", __str_dp_op(instr.op_type), __str_reg(instr.rd)) ;

    if (!is_mov(instr.op_type)) {
        append_to_dest(dest, "%s, ", __str_reg(instr.rn)) ;
    }
    
    __catstr_op2(dest, instr.op2) ;
}

const char *__str_cond(cond_e c) {
    switch (c) {
    case EQ: return "eq" ;
    case NE: return "ne" ;
    case GE: return "ge" ;
    case LT: return "lt" ;
    case GT: return "gt" ;
    case LE: return "le" ;
    case AL: return "al" ;
    }
    log_exit_failure("Unknown cond %u", c) ;
}

/// @brief concat branch instruction string to dest
void __catstr_instr_b(char **dest, instr_b instr) {
    switch (instr.tp) {
    case TP_B: 
        if (!instr.label) instr.label = "" ;
        append_to_dest(dest, "b %lx <%s>", instr.address, instr.label) ;
        break ;
    case TP_BCond: 
        if (!instr.label) instr.label = "" ;
        append_to_dest(dest, "b.%s %lx <%s>", __str_cond(instr.cond), instr.address, instr.label) ;
        break ;
    case TP_BR:
        append_to_dest(dest, "br %s", __str_reg(instr.rn)) ; 
        break ;
    }
}

static inline
const char *__str_ls_op(e_ls_op op) {
    switch (op) {
        case OP_LDR: return "ldr" ;
        case OP_STR: return "str" ;
        default:
        log_exit_failure("Unknown ls_op %u", op) ;
    }
}

/// @brief Show a load/store with immediate offset argument.
void __catstr_ls_imm(char **dest, i_ls_imm instr) {
    append_to_dest(dest, "[%s", __str_reg(instr.rn)) ;
    
    char *fmt ;
    switch(instr.idx_tp) {
    case IDX_PRE: fmt = ", #%d]!" ; break ;
    case IDX_POST: fmt = "], #%d" ; break ;
    case IDX_U_OFFSET: fmt = ", #%u]" ; break ;
    }
    append_to_dest(dest, fmt, instr.imm) ;
}

/// @brief Show a register extension
/// @param tp 
/// @return 
const char *__str_extend_e(ls_reg_extend_tp tp) {
    char *res ;
    switch (tp) {
        // case E_LS_EXTEND_UXTW: return "uxtw" ;
        case E_LS_EXTEND_LSL : return "lsl" ;
        // case E_LS_EXTEND_SXTW: return "sxtw" ;
        case E_LS_EXTEND_SXTX: return "sxtx" ;
        default:
        log_exit_failure("Unknown extend type %u", tp) ;
    }
}

/// @brief Show register extension
void __catstr_extend(char **dest, extend_t e) {
    append_to_dest(dest, "%s #%d", __str_extend_e(e.tp), e.amount) ;
}

/// @brief Show a load/store with register offset argument.
void __catstr_ls_reg(char **dest, i_ls_reg i) {
    append_to_dest(dest, "[%s, %s", __str_reg(i.rn), __str_reg(i.rm)) ;
    if (i.extend.amount != 0) __catstr_extend(dest, i.extend) ;
    append_to_dest(dest, "]") ;

}

/// @brief Show a load with literal argument.
void __catstr_ls_lit(char **dest, i_ls_lit lit) {
    if (lit.label) append_to_dest(dest, "%x <%s>", lit.lit, lit.label) ;
    else   append_to_dest(dest, "%x", lit.lit) ;
}

/// @brief Show a load/store instruction. 
void __catstr_instr_ls(char **dest, instr_ls instr) {
    append_to_dest(dest, "%s %s, ", __str_ls_op(instr.op), __str_reg(instr.rt)) ;

    switch (instr.arg_tp) {
    case LS_IMM: __catstr_ls_imm(dest, instr.imm) ; break ;
    case LS_REG: __catstr_ls_reg(dest, instr.reg) ; break ;
    case LS_LIT: __catstr_ls_lit(dest, instr.lit) ; break ;
    default: log_exit_failure("Unknown ls_arg_tp %u", instr.arg_tp) ;
    }
}


void __catstr_instr(char **dest, instr_t instr) {
    switch (instr.tp) {
        case I_DP: return __catstr_instr_dp(dest, instr.dp) ;
        case I_B:
            return __catstr_instr_b(dest, instr.b) ;
        case I_LS:
            return __catstr_instr_ls(dest, instr.ls) ;
        case I_DIRECTIVE:
            return __catstr_instr_directive(dest, instr.int_directive) ;
        case I_NOP:
            return __catstr_instr_nop(dest) ;
    }
    log_exit_failure("Unknown instruction type %u", instr.tp) ;
}

/// @brief String representation of an instruction. 
/// REMEMBER TO FREE! 
char *show_instr(instr_t instr) {
    char *res = calloc(50, sizeof(char));
    char *dest = res ;
    __catstr_instr(&dest, instr) ;
    return res ;
}

char *show_reg(reg_t r) {
    return __str_reg(r) ;
}

const char *show_shift(shift_tp s) {
    return __str_shift(s) ;
}
const char *show_dp_op(dp_op op) {
    return __str_dp_op(op) ;
}

reg_t zero_reg(bool extended) {
    return (reg_t) { .r = RZR, .extended = extended } ;
}

const reg_t NULL_REG = (reg_t) { .r = -1, .extended = -1 } ;

bool is_null_reg(reg_t r) {
    return r.r == -1 ;
}

