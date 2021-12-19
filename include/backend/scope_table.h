#ifndef SCOPE_TABLE_H
#define SCOPE_TABLE_H

#include <array.h>

struct scope_table {
        ptrdiff_t shift = 0;
        array *entries = nullptr;
};

struct var_info {
        ast_node    *node = nullptr;
        const char *ident = nullptr;
        ptrdiff_t shift = 0;
};

var_info *scope_table_find(scope_table *const table, ast_node *variable);
var_info *scope_table_add (scope_table *const table, ast_node *variable);
var_info *scope_table_top (scope_table *const table);
void      scope_table_pop (scope_table *const table);

var_info *scope_table_add_param(scope_table *const table);

void dump_array_var_info(void *item);


#endif /* SCOPE_TABLE_H */

