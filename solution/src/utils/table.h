#ifndef __TABLE_H
#define __TABLE_H

/*
 * A symbol table binding.
 */

typedef struct {
    char *(*show)(void *);
    void *value;
} table_val;

typedef struct {
    char *symbol; /// The symbol (key) for this binding.
    table_val *value; /// The associated value.
}   binding_t;

/// A symbol table node. Symbol tables are represented as ordered binary trees.
struct node {
    binding_t *binding; /// The node's binding.
    struct node *left, *right; /// The node's left and right children.
};



/// A symbol table.
typedef struct {
    /// The head of the binary tree representing the table.
    struct node *head;
    char *name;
} table_t;


/// Returned when a tree operation (insertion or lookup) succeeds.
#define TABLE_OP_SUCCESS 0

/// Returned when a tree operation (insertion or lookup) fails.
#define TABLE_OP_FAILURE 1


///  API prototypes.
#define V_BIND_CAST(type, res, lb)                                             \
    do {                                                                       \
        table_val *tv;                                                         \
        int res = find_binding(parse_table, key, &tv);                         \
        lb = tv->value;                                                        \
    } while (1);

table_t *talloc();
table_t *talloc_name(char *);
void tfree(table_t *);
void printt(table_t *);
int add_binding(table_t *, const char *, table_val *);
int add_v_bind(table_t *, const char *, char *(*show)(void *), void *);
int find_binding(table_t *, const char *, table_val **);
void *find_v_bind(table_t *table, const char *key);

typedef bool (match_func_t)(table_val *) ;
binding_t *find_val(table_t *, match_func_t) ;

#endif
