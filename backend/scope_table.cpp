#include <array.h>
#include <logs.h>
#include <assert.h>
#include <ast/tree.h>
#include <backend/scope_table.h>

void dump_array_var_info(void *item)
{
        assert(item);

        var_info *info = (var_info *)item;
        if (!info->node)
                return;

        fprintf(logs, "%s: [rx + %lu]", info->ident, info->shift);
}

var_info *scope_table_find(scope_table *const table, ast_node *variable)
{
        assert(table);
        assert(variable);

        const char *ident = ast_ident(variable);
        var_info *vars = (var_info *)(table->entries->data);
        for (size_t i = 0; i < table->entries->size; i++) {
                if (vars[i].ident == ident)
                        return &vars[i];
        }

        return nullptr;
}

var_info *scope_table_add(scope_table *const table, ast_node *variable)
{
        assert(table);
        assert(variable);

        var_info info = {0};

        info.node  = variable;
        info.ident = ast_ident(variable);
        info.shift = table->shift;

        table->shift++;
        if (variable->right)
                table->shift += ast_number(variable->right);

        return (var_info *)array_push(table->entries, &info, sizeof(var_info));
}


