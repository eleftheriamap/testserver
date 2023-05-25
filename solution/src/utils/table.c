/*
    @file table.c
    @brief Defines a symbol table data structure.

    Originally written by __ ?
*/

#include "wrapper/io.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "table.h"

/*
 * Allocates a binding for use in a symbol table. The size of the symbol must
 * be passed as an argument.
 */
binding_t *balloc(size_t symbol_size) {

    binding_t *binding = calloc(1, sizeof(binding_t));
    binding->symbol = calloc(1, symbol_size + 1);
    return binding;
}

void vfree(table_val *tv) {
    if (tv != NULL) {
        free(tv->value);
        free(tv);
    }
}

/*
 * Frees a table binding. It is safe to provide NULL to this function.
 */
void bfree(binding_t *binding) {

    if (binding != NULL) {
        free(binding->symbol);
        vfree(binding->value);
        free(binding);
    }
}

/*
 * Allocates a table node.
 */
struct node *nalloc(size_t symbol_size) {

    struct node *n = calloc(1, sizeof(struct node));
    n->binding = balloc(symbol_size);
    return n;
}

/*
 * Frees a table node. It is safe to provide NULL to this function.
 */
void nfree(struct node *n) {

    if (n != NULL) {
        bfree(n->binding);
        nfree(n->left);
        nfree(n->right);
    }
}

char *call_show(table_val *value) { return value->show(value->value); }

/*
 * Prints the contents of a table node. It is safe to pass NULL to this
 * function.
 */
void printn(struct node *n) {

    if (n != NULL) {
        printn(n->left);
        char *s = call_show(n->binding->value);
        printf("%s: %s\n", n->binding->symbol, s);
        free(s);
        printn(n->right);
    }
}

/*
 * Allocates an empty symbol table.
 */
table_t *talloc() { return (table_t *)calloc(1, sizeof(table_t)); }

table_t *talloc_name(char *name) {
    table_t *t = talloc();
    asprintf(&t->name, "%s", name);
    return t;
}

/*
 * Frees a symbol table.
 */
void tfree(table_t *table) {
    if (table->name != NULL)
        free(table->name);
    nfree(table->head);
}

/*
 * Prints a given symbol table.
 */
void printt(table_t *table) { printn(table->head); }

/*
 * A private helper function for adding a node to a symbol table.
 */
int _add_binding(struct node **n, const char *symbol, table_val *value) {

    int result;
    while (*n != NULL) {

        result = strcmp(symbol, (*n)->binding->symbol);
        if (result < 0) {

            n = &((*n)->left);
        } else if (result > 0) {

            n = &((*n)->right);
        } else {

            bfree((*n)->binding);
            (*n)->binding = balloc(strlen(symbol));
            strcpy((*n)->binding->symbol, symbol);
            (*n)->binding->value = value;
        }
    }

    *n = nalloc(strlen(symbol));
    if (*n == NULL) {

        return TABLE_OP_FAILURE;
    }

    strcpy((*n)->binding->symbol, symbol);
    (*n)->binding->value = value;

    return TABLE_OP_SUCCESS;
}

/*
 * Adds a symbol-value binding to a given symbol table. The value returned will
 * be TABLE_OP_SUCCESS if the addition was successful and TABLE_OP_FAILURE
 * otherwise.
 */
int add_binding(table_t *table, const char *symbol, table_val *value) {

    return _add_binding(&(table->head), symbol, value);
}

int add_v_bind(table_t *t, const char *key, char *(*show)(void *), void *lb) {
    table_val *tv = malloc(sizeof(table_val));
    tv->show = show;
    tv->value = lb;
    return add_binding(t, key, tv);
}

/*
 * Lookup a binding for symbol in the given table. The value will be stored in
 * the value argument. TABLE_OP_SUCCESS will be returned on success;
 * TABLE_OP_FAILURE on failure.
 */
int find_binding(table_t *table, const char *symbol, table_val **value) {
    struct node *current = table->head;
    int result;

    while (current != NULL) {
        result = strcmp(symbol, current->binding->symbol);
        if (result < 0) {
            current = current->left;
        } else if (result > 0) {
            current = current->right;
        } else {
            *value = current->binding->value;
            return TABLE_OP_SUCCESS;
        }
    }

    return TABLE_OP_FAILURE;
}




/// Very slow search through entire tree to match value.
bool _find_binding(struct node *cur, match_func_t match, binding_t **value) {
    if (!cur) return false ;
    if (match(cur->binding->value)) {
        *value = cur->binding;
        return TABLE_OP_SUCCESS;
    }
    return  _find_binding(cur->left,  match, value)
        ||  _find_binding(cur->right, match, value) ;
}

/// Very slow search through entire tree to match value.
binding_t *find_val(table_t *table, match_func_t match) {
    binding_t *b;
    int res = _find_binding(table->head, match, &b);
    if (res == TABLE_OP_FAILURE)
        return NULL ;
    else
        return b ;
}

void *find_v_bind(table_t *table, const char *key) {
    table_val *tv;
    int res = find_binding(table, key, &tv);
    if (res == TABLE_OP_FAILURE)
        return NULL;
    else
        return tv->value;
}
