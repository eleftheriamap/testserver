#include <string.h>
#include "wrapper/io.h"

#include "assembler/parser/parse_table.h"

#include "utils/table.h"
#include "utils/log.h"

static table_t *label_table ;
static table_t *op_table ;
static table_t *cond_table ;
static table_t *reg_table ;
static table_t *shift_table ;
static table_t *extend_table ;



void log_tables() {
    if (!log_labels()) return ;
    logln("Branch Labels: ") ;
    printt(label_table) ;
}



/*****************************************************************************/
// Operator Table


/**
 * @brief 
 * 
 * @param vpb 
 * @return char* 
 */
char *show_op_binding(void *vpb) {
    op_binding *pb = (op_binding*) vpb ;
    char *s = malloc(strlen(pb->key) * sizeof(char)) ;
    strcpy(s, pb->key) ;
    return s ;
}

/**
 * @brief Bind string @c key to @c op in op_table.
 * 
 * @param key 
 * @param op 
 * @param p_func 
 * @return int 
 */
int add_op_bind(const char *key, op_tok op, any_enum ast_op, p_func_t p_func) {
    op_binding *pb = malloc(sizeof(op_binding)) ;
    pb->key = key ;
    pb->op = op ;
    pb->ast_op = ast_op ;
    pb->p_func = p_func ;
    return add_v_bind(op_table, key, show_op_binding, pb) ;
}

int add_op_bind_no_ast(const char *key, op_tok op, p_func_t p_func) {
    return add_op_bind(key, op, -13, p_func) ;
}

/**
 * @brief 
 * 
 * @param key 
 * @return op_binding* The associated token. NULL if not found.
 */
op_binding *get_op_bind(const char *key) {
    return find_v_bind(op_table, key) ;
}

/**
 * @brief 
 */
void init_op_table() {
    log_init("Op Table") ;
    op_table = talloc() ;
    if (op_table == NULL) {
        log_error("parse table could not be allocated memory.") ;
        exit(EXIT_FAILURE) ;
    }
    
}



/*****************************************************************************/
// Label Table

/**
 * @brief Initialise the table of branch labels.
 * 
 * @return int 
 */
void init_label_table() {
    log_init("Label Tabe") ;
    label_table = talloc() ;
    if (label_table == NULL) {
        log_exit_failure("Label Table init fail") ;
    }
}

char *show_label_binding(void *vlb) {
    label_binding *lb = (label_binding*) vlb ;
    char *s = malloc(8 * sizeof(char)) ;
    sprintf(s, "line %lu == 0x%lx", (lb->addr / 4), lb->addr) ;
    return s ;
}

/// add label to context 
int add_label(const char *key, uint64_t addr) {
    label_binding *lb = malloc(sizeof(label_binding)) ;
    lb->key = key ;
    lb->addr = addr ;
    return add_v_bind(label_table, key, show_label_binding, lb) ;
}

label_binding *get_label(const char *key) {
    label_binding *lb = find_v_bind(label_table, key) ;
    // if (lb == NULL) {
    //     log_error("Label not found: %s", key) ;
    //     exit(EXIT_FAILURE) ;
    // }
    return lb ;
}


/*****************************************************************************/
// Enum Table


char *__show_enum(void *vcv) {
    enum_binding *cv = vcv ;
    char *s = calloc(strlen(cv->key) + 1, sizeof(char)) ;
    strcpy(s, cv->key) ;
    return s ;
}

typedef uint64_t any_enum ;

int add_enum_bind(table_t *table, const char *key, any_enum ev) {
    enum_binding *cv = malloc(sizeof(enum_binding));
    *cv = (enum_binding) {.key = key, .e=ev} ;
    table_val *tv = malloc(sizeof(table_val)) ;
    *tv = (table_val) {.show=__show_enum,.value=cv} ;

    return add_binding(table, key, tv) ;
}

/**
 * @brief Find the enum associated with @c key in the table @c table, storing result in @c res.
 * 
 * @param table Table of bindings.
 * @param key Key to search for.
 * @param res Result pointer of search.
 * @return true if found, false otherwise.
 */
bool search_enum_bind(table_t *table, const char *key, any_enum *res) {
    enum_binding *cv = find_v_bind(table, key) ;
    if(cv) *res = cv->e ; 
    return cv != NULL ;
}


/**
 * @brief Get the enum associated with @c key in the table @c table.
 * 
 * @param table table of bindings
 * @param key the search key
 * @return the enum bound to the key in @c table
 */
any_enum get_enum_bind(table_t *table, const char *key) {
    any_enum e ;
    bool search_res = search_enum_bind(table, key, &e) ;
    if (!search_res) log_exit_failure("`get_enum_bind`: enum not recognised: %s", key) ;
    return e ;
}


static any_enum __enum_match_val = 0 ;

bool __enum_match(table_val * v) {
    enum_binding *cv = v->value ;
    return cv->e == __enum_match_val ;
}

binding_t *find_enum_bind(table_t *table, any_enum e) {
    __enum_match_val = e ;
    return find_val(table, __enum_match) ;
}

/// @brief Initialises and allocates memory for a table with enum bindings.
void init_enum_table(table_t **table, char *table_name) {
    log_init("Enum Table: %p", table) ;
    *table = talloc_name(table_name) ;
    if (*table == NULL) {
        log_error("%s table talloc failed", table_name) ;
        exit(EXIT_FAILURE) ;
    }
}

/*****************************************************************************/
// Making the tables

#define MAKE_ENUM_TABLE_FUNCS(table_ptr, tok_tp, short_name)    \
    void init_ ## short_name ## _table() {                       \
        log_init(#short_name " Table") ; \
        init_enum_table(&table_ptr, # short_name) ;       \
    }                                                           \
    ENUM_TABLE_ADD(short_name, tok_tp) {                        \
        return add_enum_bind(table_ptr, key, val) ;             \
    }                                                           \
    ENUM_TABLE_GET(short_name, tok_tp) {                        \
        return get_enum_bind(table_ptr, key) ;                  \
    }                                                           \
    ENUM_TABLE_SEARCH(short_name, tok_tp, res_ptr) {              \
        return search_enum_bind(table_ptr, key, (any_enum*) res_ptr) ;        \
    }                                                           \

MAKE_ENUM_TABLE_FUNCS(cond_table, cond_tok, cond)
// MAKE_ENUM_TABLE_FUNCS(reg_table, reg_t, reg)
MAKE_ENUM_TABLE_FUNCS(shift_table, shift_tok, shift)
MAKE_ENUM_TABLE_FUNCS(extend_table, extend_tok, extend)

void init_tables() {
    init_label_table() ;
    init_op_table() ;
    init_cond_table() ;
    // init_reg_table() ;
    init_shift_table() ;
    init_extend_table() ;
}

#define tfree_safe(ptr) if (ptr != NULL) tfree(ptr)

void free_tables() {
    tfree_safe(cond_table) ;
    // tfree_safe(reg_table) ;
    tfree_safe(shift_table) ;
    tfree_safe(label_table) ;
    tfree_safe(op_table) ;
    tfree_safe(extend_table) ;
}
