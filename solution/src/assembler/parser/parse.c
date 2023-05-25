#include "utils/lib.h"
#include "assembler/parser/parse.h"
#include "common/ast.h"
#include "utils/table.h"
#include "utils/string_funcs.h"
#include "assembler/parser/lex.h"
#include "assembler/parser/parse_table.h"

#define _DELIMS ", :\n"
#define DELIMS (_DELIMS)

/*********************************************************************************************************************/
// Utils

/// @brief Returns true if @c line is a valid label declaration. 
inline
bool is_valid_label(char *line) {
    return (strchr(line, ':') != NULL) ;
}

/// @brief Returns true if @c line is a comment, whitespace or label.
inline
bool not_instr_line(char *line) {
    return is_comment(line) || is_whitespace(line) || is_valid_label(line) ;
}

/// @brief Returns true if @c line is a comment.
inline
bool is_comment(char *line) {
    for (int i = 0; line[i]; i++) {
        if (!isspace(line[i])) 
            return line[i] == '/' ;
    }
    return false ;
}

/// @brief Returns true if @c line is in the label table. 
bool is_label(char *label) {
    return get_label(label) != NULL ;
}

/**
 * @brief Add a new label to the label table.
 * 
 * @param line Line containing the label.
 * @param addr Address of the label (relative to the start of the program).
 */
void new_label(char *line, address_t addr) {
    char *tok = strtok(line, " :\n") ;
    char *s = calloc(strlen(tok) + 1, sizeof(char)) ;
    strcpy(s, tok) ;
    add_label(s, addr) ;
}

// /* Parse an immediate label */
int64_t p_imm_label(char *token) {
    log_exit_failure("p_imm_label not implemented") ;
    return 0 ;
}

/*********************************************************************************************************************/
// Parse Atoms


/// @brief Initialises a parser, splitting @c rest on the delimiters @c delims.
/// @param p Parser to initialise.
/// @param rest The string to parse.
void init_prsr_delims(prsr *p, char **rest, const char *delims) {
    p->rest = rest ;
    p->tok = strtok_r(*(p->rest), delims, p->rest);
}

/// @brief Initialises a parser, splitting @c rest on the default delimiters.
/// @param p Parser to initialise.
/// @param rest The string to parse.
void init_prsr(prsr *p, char **rest) {
    init_prsr_delims(p, rest, DELIMS) ;
}


/// @brief Advances the parser to the next token.
/// @param p The parser state.
void next_tok_p(prsr *p) {
    p->tok = strtok_r(NULL, DELIMS, p->rest);
}


/// @brief Parses a register 
reg_t p_reg(prsr *p) {
    reg_t res ;

    strtolower(p->tok) ;

    res.extended = (p->tok[0] != 'w') ;
    if (streq(p->tok, "sp")) { res.r = SP ; }
    else if (streq(p->tok, "pc")) { res.r = PC ; }
    else if (streq(p->tok + sizeof(char), "zr")) { res.r = RZR ; }
    else {
        res.r = atoi(p->tok + sizeof(char)) ;
        if (res.r > 30) log_exit_failure("Invalid register %s", p->tok) ;
    }
    next_tok_p(p) ; 
    return res ;
}

/// @brief Parse a signed immediate
imm p_imm(prsr *p) {
    if (p->tok[0] == '#') p->tok++ ;
    uint64_t x;
    if (strlen(p->tok) >= 3 && p->tok[0] == '0' && tolower(p->tok[1]) == 'x') {
        sscanf(p->tok, "0x%lx", &x) ;
    } else {
        sscanf(p->tok, "%ld", &x);
    }
    imm res = x ;
    next_tok_p(p) ;
    return res ;
}

/// @brief Parses a hash symbol followed by an immediate. 
imm p_hash_imm(prsr *p) {
    ASSERT_M(p->tok[0] == '#', "Expected #") ;
    p->tok++ ;
    return p_imm(p) ;
}

/*********************************************************************************************************************/
// Parse Shifts

/// @brief Parses a shift 
shift_arg p_shift(prsr *p) {
    shift_arg dest;
    strtolower(p->tok) ;
    dest.tp = get_shift_bind(p->tok) ;
    next_tok_p(p) ;
    dest.amount = p_hash_imm(p) ;
    return dest;
}

/// @brief Checks if a token is a shift
bool is_shift(char *token) {
    any_enum tp ;
    char *tok ;
    asprintf(&tok, "%s", token) ;
    strtolower(tok) ;
    bool res = search_shift_bind(tok, &tp) ;
    log_free(tok) ;
    return res ;
}

/// @brief Tries to parse a shift; returns @c DEFAULT_SHIFT on failure. 
static inline
shift_arg p_try_sh(prsr *p) {
    if (p->tok && is_shift(p->tok)) return p_shift(p) ;
    else return DEFAULT_SHIFT ;
}

/*********************************************************************************************************************/
// Parse 2nd Operand

/* Parse a (possibly shifted) immediate value */ 
op2_imm p_imm_sh(prsr *p) {
    op2_imm dest;
    if (p->tok[0] == '=') {dest.imm = p_imm_label(p->tok) ; return dest; }
    
    ASSERT (p->tok[0] == '#') ;
    dest.imm = p_hash_imm(p) ;
    dest.sh = p_try_sh(p) ;
    
    return dest;
}

/// @brief Parse a shifted register
op2_reg p_reg_sh(prsr *p) {
    op2_reg dest ;
    dest.rm = p_reg(p) ;
    dest.sh = p_try_sh(p);
    return dest ;
}

/// @brief Checks if `c` occurs in the string `cs`.
static inline
bool is_one_of(char c, const char *cs) {
    return strchr(cs, c) != NULL ;
}

/// @brief Parse an operand of a data processing instruction
op2_t p_dp_op2 (prsr *p) {
    op2_t dest;
    if (is_one_of(p->tok[0], "=#")) {
        // Op2 is a (shifted) immediate
        dest.type   = OP2_IMM_SH;
        dest.imm_sh = p_imm_sh(p);
    } else {
        // Op2 is a (shifted) register
        dest.type   = OP2_REG_SH;
        dest.reg_sh = p_reg_sh(p);
    }

    return dest;
}

/*********************************************************************************************************************/
// Parse each instruction case

/**
 * @brief Parse a data processing instruction.
 * 
 * @param dest The instruction to parse into.
 * @param line The line to parse.
 * @param op_code The already parsed mnemonic of the current instruction.
 */
void p_dp (instr_t *dest, prsr *p, dp_op *op_code) {
    dest->tp = I_DP ;

    dest->dp.op_type = *op_code ;
    next_tok_p(p) ;
    
    dest->dp.rd = p_reg(p);
    dest->dp.rn = p_reg(p);
    dest->dp.op2 = p_dp_op2(p);
}

/**
 * @brief Parse a data processing instruction with only one operand.
 * 
 * @param dest The instruction to parse into.
 * @param p The line to parse.
 * @param op_code The already parsed mnemonic of the current instruction.
 */
void p_dp_1 (instr_t *dest, prsr *p, dp_op *op_code, reg_t *reg, reg_t *rzr) {
    dest->tp = I_DP ;

    dest->dp.op_type = *op_code ;
    next_tok_p(p) ;
    
    *reg = p_reg(p) ;
    *rzr = zero_reg(dest->dp.rn.extended) ;

    dest->dp.op2 = p_dp_op2(p);
}

void p_dp_1_rd_zr(instr_t *dest, prsr *p, dp_op *op_code) {
    p_dp_1(dest, p, op_code, &(dest->dp.rn), &(dest->dp.rd)) ;
}

void p_dp_1_rn_zr(instr_t *dest, prsr *p, dp_op *op_code) {
    p_dp_1(dest, p, op_code, &(dest->dp.rd), &(dest->dp.rn)) ;
}

/**
 * @brief Parse a mov(n|z|k) instruction.
 * 
 * @param dest The instruction to parse into.
 * @param p The parser.
 * @param op_code The already parsed mnemonic of the current instruction.
 */
void p_mov(instr_t *dest, prsr *p, dp_op *op_code) {
    dest->tp = I_DP ;

    dest->dp.op_type = *op_code ;
    next_tok_p(p) ;
    
    dest->dp.rd = p_reg(p) ;
    dest->dp.rn = NULL_REG ;
    dest->dp.op2.type = OP2_IMM_SH ;
    dest->dp.op2.imm_sh = p_imm_sh(p);
}

/*********************************************************************************************************************/
// Parse Branch instructions

/**
 * @brief Parse a label or immediate.
 * 
 * @param p The parser state.
 * @param lb If the token is a label, this will be set to the label binding.
 * @return The address of the label or the immediate value.
 */
address_t p_label_or_imm(prsr *p, label_binding **lb) {
    if (strlen(p->tok) < 1) log_exit_failure("p_label_or_imm: tok is empty") ;

    // Is it a label?
    label_binding *b = get_label(p->tok) ;
    if (b) {
        if (lb) *lb = b ;
        return b->addr ;
    } 

    // It's not a label, so an immediate:
    return p_imm(p) ;
}

/**
 * @brief Parse a branch instruction.
 * 
 * @param dest The instruction to parse into.
 * @param line The line to parse.
 * @param op_code The already parsed mnemonic of the current instruction.
 */
void p_branch(instr_t *dest, prsr *p, e_b_tp *branch_tp) {
    dest->tp = I_B ;
    dest->b.tp = *branch_tp ;
    
    next_tok_p(p) ;

    if (dest->b.tp == TP_BR) {
        dest->b.rn = p_reg(p) ;
    } else {
        label_binding *lb = NULL ;
        dest->b.address = p_label_or_imm(p, &lb) ;
        if (lb) dest->b.label = lb->key ;
        else  dest->b.label = NULL ;
    }
}

/**
 * @brief Parse a conditional branch instruction.
 * 
 * @param dest The instruction to parse into.
 * @param p The parser state.
 * @param cond The condition of the branch.
 */
void p_b_cond(instr_t *dest, prsr *p, cond_e *cond) {
    dest->tp = I_B ;
    dest->b.cond = *cond ;
    p_branch(dest, p, &(e_b_tp){TP_BCond}) ;
}

/**
 * @brief Try to run a parser function if a condition is met.
 * 
 * @param p The current parser state.
 * @param res Pointer to save the result of the parser function.
 * @param cond_func The condition function, which determines if the parser function should be run.
 * @param cond_aux Auxiliary data for the condition function.
 * @param p_func The parser function to run.
 * @param p_aux Auxiliary data for the parser function.
 */
void __p_try_cond(
    prsr *p, void **res, 
    bool (cond_func)(prsr *, void *), void *cond_aux,
    void *(p_func)(prsr *, void *), void *p_aux) {
    
    if (p->tok && (!cond_func || cond_func(p, cond_aux))) {
        *res = p_func(p, p_aux) ;
    }
}

/**
 * @brief Parse an extension mnemonic.
 * 
 * @param p The parser state.
 * @return ls_reg_extend_tp The extension mnemonic.
 */
ls_reg_extend_tp p_extend_tp(prsr *p) {
    ls_reg_extend_tp e_tp ;
    e_tp = get_extend_bind(p->tok) ;
    next_tok_p(p) ;
    return e_tp ;
}

/**
 * @brief Parse an extension with an optional immediate amount.
 * 
 * @param p The current parser state.
 * @return extend_t The parsed extension.
 */
extend_t p_extend(prsr *p) {
    extend_t res ;
    res.tp = p_extend_tp(p) ;
    res.amount = (p->tok) ? p_imm(p) : 0 ;
    return res ;
}

#define DEFAULT_EXTEND ((extend_t){ .tp = E_LS_EXTEND_LSL, .amount = 0 })

/**
 * @brief Try to parse an extension, returning a default value if the token is empty.
 * 
 * @param p The current parser state.
 * @return extend_t The parsed extension, defaulting to LSL 0.
 */
extend_t p_try_extend(prsr *p) {
    extend_t res ;
    if (p->tok) res = p_extend(p) ;
    else res = DEFAULT_EXTEND ;
    return res ;
}

/*********************************************************************************************************************/
// Parse Load/Store instructions

/**
 * @brief Determine the type of the index in a load/store instruction, assuming it is an immediate offset.
 * This edits the string in the parser, removing the brackets and exclamation mark, 
 * making it easier to parse the remaining operands.
 *  
 * Thus, this function is quite lengthy, and does some hacky checks to determine the indexing/offset type.
 * 
 * @param p The parser state.
 * @return e_ls_idx The index type.
 */
e_ls_idx determine_ls_idx_type(prsr *p) {
    e_ls_idx idx ;
    char *excl_idx = strchr(*(p->rest), '!' ) ;
    char *sqr_idx = strchr(*(p->rest), ']' ) ;
    if (excl_idx) {
        idx = IDX_PRE ;
        // Don't read after `]!`
        *excl_idx = '\0' ;
        *sqr_idx = '\0' ;
    }   else if (strstr(*(p->rest), "],")) {
        idx = IDX_POST ;
        // strtok will consume the ",," as a single delimiter :p
        *sqr_idx = ',' ; 
    } else {
        idx = IDX_U_OFFSET ;
        *sqr_idx = '\0' ;
    }
    char *sqr_open_idx = strchr(*(p->rest), '[' ) ;
    *sqr_open_idx = ' ' ;
    return idx ;
}

/**
 * @brief Parse the rest of a load/store instruction with an immediate offset.
 * 
 * @param dest The instruction to parse into.
 * @param p The parser state.
 * @param rn The base address register.
 * @param idx The type of index to use.
 */
static inline
void p_ls_imm(instr_t *dest, prsr *p, reg_t rn, e_ls_idx idx) {
    dest->ls.arg_tp = LS_IMM ;
    dest->ls.imm.idx_tp = idx ;
    dest->ls.imm.rn = rn ;
    dest->ls.imm.imm = p->tok ? p_imm(p)  : 0 ;
}

/**
 * @brief Parse the rest of a load/store instruction with a register offset.
 * 
 * @param dest The instruction to parse into.
 * @param p The parser state.
 * @param rn Base address register.
 */
static inline
void p_ls_reg(instr_t *dest, prsr *p, reg_t rn) {
    dest->ls.arg_tp = LS_REG ;
    dest->ls.reg.rn = rn ;
    dest->ls.reg.rm = p_reg(p) ;
    dest->ls.reg.extend = p_try_extend(p) ;
}

/**
 * @brief Parse a load/store instruction with an immediate or register offset.
 * 
 * @param dest The instruction to parse into.
 * @param p The parser state.
 */
void p_ls_imm_or_reg(instr_t *dest, prsr *p) {
    e_ls_idx idx = determine_ls_idx_type(p) ;

    next_tok_p(p) ;
    dest->ls.rt = p_reg(p) ;
    reg_t rn = p_reg(p) ;

    if (p->tok && (tolower(p->tok[0]) == 'x' || tolower(p->tok[0]) == 'w'))
        p_ls_reg(dest, p, rn) ;
    else p_ls_imm(dest, p, rn, idx) ;
}

/**
 * @brief Parse a load/store literal instruction.
 * 
 * @param dest The instruction to save result into.
 * @param p The parser state.
 */
void p_ldr_lit(instr_t *dest, prsr *p) {
    dest->ls.arg_tp = LS_LIT ;
    next_tok_p(p) ;
    dest->ls.rt = p_reg(p) ;

    label_binding *lb = NULL ;
    dest->ls.lit.lit = p_label_or_imm(p, &lb) ;
    dest->ls.lit.label = lb->key ;
}

/**
 * @brief Parse a load/store instruction.
 * 
 * @param dest The instruction to save result into.
 * @param p The parser state.
 * @param ls_op The type of load/store instruction.
 */
void p_ls(instr_t *dest, prsr *p, e_ls_op *ls_op) {
    bool is_lit = strchr(*(p->rest), '[') == NULL ;
    dest->tp = I_LS ;
    dest->ls.op = *ls_op ;

    if (is_lit) p_ldr_lit(dest, p) ;
    else p_ls_imm_or_reg(dest, p) ;
}

/*********************************************************************************************************************/
// Multiplication instructions

/**
 * @brief Parse a multiplication instruction.
 * 
 * @param dest The instruction to save result into.
 * @param p The parser state.
 * @param mul_op The type of multiplication instruction.
 */
void p_mul(instr_t *dest, prsr *p, dp_op *mul_op) {
    dest->tp = I_DP ;
    dest->dp.op_type = *mul_op ;
    next_tok_p(p) ;
    dest->dp.rd = p_reg(p) ;
    dest->dp.rn = p_reg(p) ;
    dest->dp.op2.type = OP2_MUL ;
    dest->dp.op2.mul.rm = p_reg(p) ;
    dest->dp.op2.mul.ra = p->tok ? p_reg(p) : zero_reg(dest->dp.rd.extended) ;
}

/*********************************************************************************************************************/
// Parse Misc Instructions

/**
 * @brief Parse a `.int X` directive.
 * 
 * @param dest The instruction to save result into.
 * @param p The parser state.
 * @param aux Unused auxiliary parameter.
 */
void p_directive(instr_t *dest, prsr *p, UNUSED any_enum *aux) {
    dest->tp = I_DIRECTIVE ;

    next_tok_p(p) ;
    if (strlen(p->tok) >= 2 && p->tok[0] == '0' && tolower(p->tok[1]) == 'x') {
        sscanf(p->tok, "0x%x", &(dest->int_directive)) ;
    } else {
        sscanf(p->tok, "%u", &(dest->int_directive)) ;
    }
}

/**
 * @brief Parse a `nop` instruction.
 * 
 * @param dest Instruction to save result into.
 * @param p The parser state. 
 * @param aux Ignored auxiliary parameter.
 */
void p_nop(instr_t *dest, prsr *p, UNUSED any_enum *aux) {
    // char *tok = INIT_TOKEN(line) ;
    ASSERT_M (streq(p->tok, "nop"), "p_nop called on not nop: %s", p->tok) ;
    dest->tp=I_NOP ;
}

/*********************************************************************************************************************/
// Parse an instruction; the main function

/**
 * @brief Parses a string line into an instruction AST node.
 * 
 * @param line The string to be parsed.
 * @return instr_t The parsed instruction.
 */
instr_t p_instr(char *line) {
    instr_t dest ;
    
    prsr *p = &(prsr) {} ;
    init_prsr(p, &line) ;
    
    strtolower(p->tok) ;
    op_binding *pb = get_op_bind(p->tok) ;
    if (!pb) log_exit_failure("Parsing: Unknown mnemonic: %s", p->tok) ;

    (pb->p_func)(&dest, p, &pb->ast_op) ;


    return dest;
}

/*********************************************************************************************************************/
// Add bindings to the tables

/// @brief Emits failure message when @c i is non-zero; 
/// for when adding table bindings fails.
int check_bindings_res(int i, const char *table_name) {
    if (i) log_exit_failure("failed to add bindings to %s table", table_name) ;
    return i ;
}

/// @brief Add all the mnemonic string to token/func bindings to the table.
/// @return 0 on success, non-zero when at least one binding failed.
int add_op_bindings() {
    int i = 0; 
    i |= add_op_bind("add",  TOK_ADD, OP_ADD, (p_func_t) p_dp) ;
    i |= add_op_bind("sub",  TOK_SUB, OP_SUB, (p_func_t) p_dp) ;
    i |= add_op_bind("adds", TOK_ADDS, OP_ADDS, (p_func_t) p_dp) ;
    i |= add_op_bind("subs", TOK_SUBS, OP_SUBS, (p_func_t) p_dp) ;
    i |= add_op_bind("cmp", TOK_CMP, OP_CMP, (p_func_t) p_dp_1_rd_zr) ;
    i |= add_op_bind("cmn", TOK_CMN, OP_CMN, (p_func_t) p_dp_1_rd_zr) ;

    i |= add_op_bind("and", TOK_AND, OP_AND, (p_func_t) p_dp) ;
    i |= add_op_bind("bic", TOK_BIC, OP_BIC, (p_func_t) p_dp) ;
    i |= add_op_bind("orr", TOK_ORR, OP_ORR, (p_func_t) p_dp) ;
    i |= add_op_bind("orn", TOK_ORN, OP_ORN, (p_func_t) p_dp) ;
    i |= add_op_bind("eor", TOK_EOR, OP_EOR, (p_func_t) p_dp) ;
    i |= add_op_bind("eon", TOK_EON, OP_EON, (p_func_t) p_dp) ;

    i |= add_op_bind("ands",TOK_ANDS, OP_ANDS, (p_func_t) p_dp) ;
    i |= add_op_bind("bics",TOK_BICS, OP_BICS, (p_func_t) p_dp) ;

    i |= add_op_bind("tst",TOK_TST, OP_TST, (p_func_t) p_dp_1_rd_zr) ;

    i |= add_op_bind("mov",TOK_MOV, OP_ORR, (p_func_t) p_dp_1_rn_zr) ;
    i |= add_op_bind("movn",TOK_MOVN, OP_MOVN, (p_func_t) p_mov) ;
    i |= add_op_bind("movz",TOK_MOVZ, OP_MOVZ, (p_func_t) p_mov) ;
    i |= add_op_bind("movk",TOK_MOVK, OP_MOVK, (p_func_t) p_mov) ;

    i |= add_op_bind("mul",TOK_MUL, OP_MADD, (p_func_t) p_mul) ;
    i |= add_op_bind("madd",TOK_MADD, OP_MADD, (p_func_t) p_mul) ;
    i |= add_op_bind("msub",TOK_MSUB, OP_MSUB, (p_func_t) p_mul) ;
    i |= add_op_bind("mneg",TOK_MNEG, OP_MSUB, (p_func_t) p_mul) ;

    i |= add_op_bind("ldr",TOK_LDR, OP_LDR, (p_func_t) p_ls) ;
    i |= add_op_bind("str",TOK_STR, OP_STR, (p_func_t) p_ls) ;

    i |= add_op_bind("b", TOK_B, TP_B, (p_func_t) p_branch) ;
    i |= add_op_bind("br", TOK_BR, TP_BR , (p_func_t) p_branch) ;

    i |= add_op_bind("b.eq", TOK_BCond, EQ, (p_func_t) p_b_cond) ;
    i |= add_op_bind("b.ne", TOK_BCond, NE, (p_func_t) p_b_cond) ;
    i |= add_op_bind("b.ge", TOK_BCond, GE, (p_func_t) p_b_cond) ;
    i |= add_op_bind("b.lt", TOK_BCond, LT, (p_func_t) p_b_cond) ;
    i |= add_op_bind("b.gt", TOK_BCond, GT, (p_func_t) p_b_cond) ;
    i |= add_op_bind("b.le", TOK_BCond, LE, (p_func_t) p_b_cond) ;
    i |= add_op_bind("b.al", TOK_BCond, AL, (p_func_t) p_b_cond) ;


    i |= add_op_bind(".int",TOK_DotInt, 0, (p_func_t) p_directive) ;
    i |= add_op_bind("nop", TOK_NOP, 0, (p_func_t) p_nop) ;

    return check_bindings_res(i, "op") ;
}

/// @brief Add all the shift string to token bindings to the table.
/// @return 0 on success, non-zero when at least one binding failed.
int add_shift_bindings() {
    int i = 0 ;
    i |= add_shift_bind("lsl", TOK_LSL) ;
    i |= add_shift_bind("lsr", TOK_LSR) ;
    i |= add_shift_bind("asr", TOK_ASR) ;
    i |= add_shift_bind("ror", TOK_ROR) ;
    return check_bindings_res(i, "shift") ;
}

/// @brief Add all the extend string to token bindings to the table.
/// @return 0 on success, non-zero when at least one binding failed. 
int add_extend_bindings() {
    int i = 0 ;
    i |= add_extend_bind("lsl", TOK_EXTEND_LSL) ;
    i |= add_extend_bind("sxtx", TOK_EXTEND_SXTX) ;
    return check_bindings_res(i, "extend") ;
}

/// @brief Initialise the parser.
int init_parsing_tables() {
    log_init("Initialising parser") ;
    init_tables() ;
    int i = add_op_bindings() ;
    i |= add_shift_bindings() ;
    i |= add_extend_bindings() ;
    return i ;
}

void free_parsing_tables() {
    free_tables() ;
}
