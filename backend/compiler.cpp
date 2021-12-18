#include <stdio.h>
#include <stdlib.h>
#include <logs.h>
#include <array.h>
#include <iommap.h>
#include <assert.h>
#include <stack.h>
#include <ast/tree.h>
#include <ast/keyword.h>
#include <backend/scope_table.h>

struct symbol_table {
        scope_table *local  = nullptr;
        scope_table *global = nullptr;
        array       *func   = nullptr;
};

struct func_info {
        ast_node    *node = nullptr;
        const char *ident = 0;
        size_t n_params   = 0;
};

static const size_t BUFSIZE = 128;
static char BUFFER[BUFSIZE] = {0};

static const char *const RETURN_REG = "ax";
static const char *const GLOBAL_REG = "cx";
static const char *const LOCAL_REG  = "bx";

static int       keyword(ast_node *node);
static double    *number(ast_node *node);
static const char *ident(ast_node *node);

static inline const char *number_str(double num);

static ast_node *success(ast_node *root);
static ast_node *syntax_error(ast_node *root);

static inline const char *id(const char *name, void *id);

static void dump_array_function(void *item);

static inline void LABEL(const char *arg);
static inline void WRITE(const char *arg);

#define CMD(name, code, str, hash) \
static inline void name(const char *arg = nullptr);
#include <commands>
#undef CMD

#define require(node, _ast_type)                \
        do {                                    \
        if (node && keyword(node) != _ast_type) \
                return syntax_error(root);      \
        } while (0)

#define require_ident(node)                       \
        do {                                      \
        if (node && node->type != AST_NODE_IDENT) \
                return syntax_error(root);        \
        } while (0)

#define require_number(node)                       \
        do {                                       \
        if (node && node->type != AST_NODE_NUMBER) \
                return syntax_error(root);         \
        } while (0)

static inline const char *global_variable(var_info *var, ptrdiff_t shift = 0);
static inline const char *local_variable (var_info *var, ptrdiff_t shift = 0);
static inline const char *memory(const char *reg, ptrdiff_t shift);

static const char *create_variable(ast_node *variable,
                                   scope_table *const local_table,       
                                   scope_table *const global_table);

static const char *get_variable(ast_node *variable,
                                scope_table *const local_table,       
                                scope_table *const global_table);

static const char *find_variable(ast_node *variable,
                                 scope_table *const local_table,       
                                 scope_table *const global_table);

static ast_node *statement_rule(ast_node *root, array *const func_table,
                                scope_table *const local_table,
                                scope_table *const global_table);

static ast_node *declare_function(ast_node *root, array *const func_table);

static ast_node *assign_rule(ast_node *root, array *const func_table,
                             scope_table *const local_table,
                             scope_table *const global_table);

static ast_node *expression_rule(ast_node *root, array *const func_table,
                                 scope_table *const local_table,
                                 scope_table *const global_table,
                                 ptrdiff_t shift = 0);

static ast_node *variable_rule(ast_node *root, array *const func_table,
                               scope_table *const local_table,
                               scope_table *const global_table);

static ast_node *main_brunch_assign(ast_node *root, 
                                    array *const func_table,
                                    scope_table *const global_table);

static ast_node *main_brunch_functions(ast_node *root, array *const func_table);

static func_info *find_function(ast_node *call,
                                   array *const func_table);

static ast_node *declare_function_variable(ast_node *root,
                                           scope_table *const local_table,
                                           scope_table *const global_table);

static ast_node *call_rule(ast_node *root, array *const func_table,
                           scope_table *const local_table,
                           scope_table *const global_table);

static ast_node *define_function_rule(ast_node *root, array *const func_table,
                                      scope_table *const local_table,
                                      scope_table *const global_table);

static ast_node *return_rule(ast_node *root, array *const func_table,
                             scope_table *const local_table,
                             scope_table *const global_table);

static ast_node *define_functions(ast_node *root, array *const func_table,
                                  scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(global_table);
        ast_node *error = nullptr;
$$
        if (root->left) {
$$
                error = define_functions(root->left, func_table, global_table);
$$
                if (error) {
$$
                        return error;
                }
        }
$$
        if (!root->right)
                return syntax_error(root);
$$

        if (root->right->type != AST_NODE_KEYWORD || 
            ast_keyword(root->right) != AST_DEFINE)
                return success(root);
$$

        scope_table st = {0};
        array entries  = {0};
        st.entries = &entries;
$$

        error = define_function_rule(root->right, func_table, &st, global_table);
        if (!error)
                dump_array(&entries, sizeof(var_info), dump_array_var_info);

        free_array(&entries, sizeof(var_info));
        return error;
}

static ast_node *define_function_rule(ast_node *root, array *const func_table,
                                      scope_table *const local_table,
                                      scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

        $$
        require(root, AST_DEFINE);

        WRITE(";DEFINE FUNCTION\n");
        $$
        func_info *func = find_function(root->left->left, func_table);
        $$
        if (!func)
                return syntax_error(root);

        $$
        WRITE("\n\n");
        LABEL(func->ident);
        if (root->left && root->left->right) {
        $$
                error = declare_function_variable(root->left->right, 
                                                  local_table, global_table);
        $$
                if (error)
                        return error;
        }
        $$

        if (!root->right)
                return syntax_error(root);
        $$

        error = statement_rule(root->right, func_table, local_table, global_table);
        $$
        if (error)
                return error;

        $$
        return success(root);
}

static ast_node *return_rule(ast_node *root, array *const func_table,
                             scope_table *const local_table,
                             scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

        require(root, AST_RETURN);
        error = expression_rule(root->right, func_table, local_table, global_table);
        if (error)
                return error;

        POP(RETURN_REG);
        RET();
        return success(root);
}

static ast_node *if_rule(ast_node *root, array *const func_table,
                         scope_table *const local_table,
                         scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

        require(root, AST_IF);

        WRITE("; IF");

        return success(root);
}
static ast_node *condition_rule(ast_node *root, array *const func_table,
                                scope_table *const local_table,
                                scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);

        WRITE("; CONDITION");


        return success(root);
}

static ast_node *while_rule(ast_node *root, array *const func_table,
                            scope_table *const local_table,
                            scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

        require(root, AST_WHILE);
        WRITE("; WHILE");

        LABEL(id("while", root));
        if (!root->left)
                return syntax_error(root);

        error = condition_rule(root->left, func_table, local_table, global_table);
        if (error)
                return error;

        PUSH("0");
        JE(id("while_end", root));

        error = statement_rule(root->right, func_table, local_table, global_table);
        if (error)
                return error;

        JMP(id("while", root));
        LABEL(id("while_end", root));

        return success(root);
}


static ast_node *statement_rule(ast_node *root, array *const func_table,
                                scope_table *const local_table,
                                scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

$$
        require(root, AST_STMT);
$$
        if (root->left) {
$$
                error = statement_rule(root->left, func_table, 
                                      local_table, global_table);
$$
                if (error)
                        return error;
        }
$$

        if (!root->right || root->right->type != AST_NODE_KEYWORD)
                return syntax_error(root);
$$
        switch (ast_keyword(root->right)) {
        case AST_ASSIGN:
$$
                return assign_rule(root->right, func_table, 
                                   local_table, global_table);
        case AST_IF:
$$
                return if_rule(root->right, func_table, 
                               local_table, global_table);
        case AST_WHILE:
$$
                return while_rule(root->right, func_table, 
                                  local_table, global_table);
        case AST_CALL:
$$
                return call_rule(root->right, func_table, 
                                 local_table, global_table);
        case AST_RETURN:
$$
                return return_rule(root->right, func_table, 
                                   local_table, global_table);
        default:
                return syntax_error(root);
        }
}

static ast_node *declare_function_variable(ast_node *root,
                                           scope_table *const local_table,
                                           scope_table *const global_table)
{
        assert(root);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

        require(root, AST_PARAM);
        if (root->left) {
                error = declare_function_variable(root->left, local_table, global_table);
                if (error)
                        return error;
        }

        const char *ident = create_variable(root->right, 
                                            local_table, global_table);
        if (!ident)
                return syntax_error(root);

        return success(root);
}


static ast_node *assign_rule(ast_node *root, array *const func_table,
                             scope_table *const local_table,
                             scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

        $$
        require(root, AST_ASSIGN);
        error = expression_rule(root->right, func_table, 
                                local_table, global_table);
        if (error)
                return error;

        $$
        require_ident(root->left);
        const char *ident = get_variable(root->left, local_table, global_table);
        if (!ident)
                return syntax_error(root);
        $$

        POP(ident);
        $$
        return success(root);
}

static ast_node *param_rule(ast_node *root, array *const func_table,
                           scope_table *const local_table,
                           scope_table *const global_table,
                           ptrdiff_t shift = 0)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);

        ast_node *error = nullptr;
        if (root->left) {
                error = param_rule(root->left, func_table, 
                                   local_table, global_table, args - 1);
                if (error)
                        return error;
        }

        PUSH(";PARAM");
        error = expression_rule(root->right, func_table, local_table, global_table);
        if (error)
                return error;

        POP(memory(LOCAL_REG, args));

        return success(root);
}

static ast_node *call_rule(ast_node *root, array *const func_table,
                           scope_table *const local_table,
                           scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        assert(local_table);
        assert(global_table);
        ast_node *error = nullptr;

        require(root, AST_CALL);
        func_info *func = find_function(root->left, func_table);
        if (!func)
                return syntax_error(root);

        WRITE("; call func ");
        WRITE(func->ident);

        size_t n_params = 0;
        ast_node *param = root->right;
        while (param) {
                param = param->left;
                n_params++;
        }

        if (func->n_params != n_params)
                return syntax_error(root);

        if (root->right) {
                error = param_rule(root->right, func_table, local_table, 
                                   global_table, func->n_params - 1 + local_table->shift);
                if (error)
                        return error;
        }

        PUSH(LOCAL_REG);
        PUSH(number_str(local_table->shift));
        ADD();
        POP(LOCAL_REG);

        CALL(func->ident);

        PUSH(LOCAL_REG);
        PUSH(number_str(local_table->shift));
        SUB();
        POP(LOCAL_REG);

        return success(root);
}

static ast_node *expression_rule(ast_node *root, array *const func_table,
                                scope_table *const local_table,
                                scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        ast_node *error = nullptr;

$$
        if (root->type == AST_NODE_KEYWORD && ast_keyword(root) == AST_CALL)
                return call_rule(root, func_table, local_table, global_table);
$$
        if (root->left) {
                error = expression_rule(root->left, func_table, 
                                        local_table, global_table);
                if (error)
                        return error;
        }
$$

        if (root->right) {
                error = expression_rule(root->right, func_table, 
                                        local_table, global_table);
                if (error)
                        return error;
        }
$$

        const char *ident = nullptr;
        switch (root->type) {
        case AST_NODE_NUMBER:
                PUSH(number_str(ast_number(root)));
                return success(root);
        case AST_NODE_IDENT:
                ident = find_variable(root, local_table, global_table);
                if (!ident)
                        return syntax_error(root);
                PUSH(ident);
                return success(root);
        default:
                break;
        }
        $$

$$
        switch (ast_keyword(root)) {
        case AST_ADD:
                ADD();
                return success(root);
        case AST_SUB:
                SUB();
                return success(root);
        case AST_MUL:
                MUL();
                return success(root);
        case AST_DIV:
                DIV();
                return success(root);
        case AST_POW:
                POW();
                return success(root);
        default:
                break;
        }

        return syntax_error(root);
}

static ast_node *variable_rule(ast_node *root, array *const func_table,
                              scope_table *const local_table,
                              scope_table *const global_table)
{
        assert(root);
        assert(func_table);

        require_ident(root);
        const char *ident = create_variable(root, local_table, global_table);
        if (!ident)
                return syntax_error(root);

        POP(ident);

        return success(root);
}

static ast_node *main_brunch_assign(ast_node *root, 
                                    array *const func_table,
                                    scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        ast_node *error = nullptr;

        require(root, AST_STMT);

        $$
        if (root->left) {
                error = main_brunch_assign(root->left, func_table, global_table);
                if (error)
                        return error;
        }

        $$
        if (keyword(root->right) != AST_ASSIGN)
                return success(root); 

        $$
        error = assign_rule(root->right, func_table, global_table, global_table);
        if (error)
                return error;

        return success(root);
}

static ast_node *declare_function(ast_node *root, array *const func_table)
{
        assert(root);

        ast_node *function = root->left;

        func_info *funcs = (func_info *)func_table->data;
        const char *ident = ast_ident(function->left);
        for (size_t i = 0; i < func_table->size; i++) {
                if (funcs[i].ident == ident)
                        return syntax_error(root);
        }

        func_info info = {0};

        info.node = function;
        info.ident = ident;
        info.n_params = 0;

        ast_node *param = function->right;
        while (param) {
                info.n_params++;
                param = param->left;
        }

        array_push(func_table, &info, sizeof(func_info));
        return success(root);
}

static ast_node *main_brunch_functions(ast_node *root, array *const func_table)
{
        assert(root);
        assert(func_table);
        ast_node *error = nullptr;

        if (keyword(root) != AST_STMT)
                return syntax_error(root);

        if (root->left) {
                error = main_brunch_functions(root->left, func_table);
                if (error)
                        return error;
        }

        if (keyword(root->right) != AST_DEFINE)
                return success(root);

        return declare_function(root->right, func_table);
}

int main()
{
        mmap_data md = {0};
        int error = mmap_in(&md, "tree"); 
        if (error)
                return EXIT_FAILURE;

        array idents = {0};

        /*
        PUSH(number(221));
        PUSH(number(2));
        ADD();
        JMP("label");
        JNE();
        AB();
        LABEL("label");

        PUSH(number(3));
        PUSH(variable("root"));
        //CALL(function(root));
        //PUSH(variable(root));

        CALL(id("while", &md));

        LABEL(id("while", &md));
        */

        char *r = md.buf;
        ast_node *rt = read_ast_tree(&r, &idents);
        if (rt) {
                dump_tree(rt);
                dump_array(&idents, sizeof(char *), array_string);
        }

        mmap_free(&md);

        scope_table gst = {0};
        array gentries = {0};
        gst.entries = &gentries;

        array func_table = {0};
        main_brunch_functions(rt, &func_table);
        //declare_func(rt->right, &func_table);
        dump_array(&func_table, sizeof(func_info), dump_array_function);
        main_brunch_assign(rt, &func_table, &gst);
        dump_array(gst.entries, sizeof(var_info), dump_array_var_info);

        fprintf(logs, html(red, "OMGOD\n"));
        define_functions(rt, &func_table, &gst);
        //define_function_rule(rt->left->left->left->right, &func_table, &lst, &gst);
        //dump_array(lst.entries, sizeof(var_info), dump_array_var_info);

        //PUSH(create_variable(rt->left->left->left->right->left, &st, &st));
        //POP(find_variable(rt->left->left->left->right->left, &st, &st));
        //PUSH(create_variable(rt->left->left->right->left, &st, &st));

        free_array(&func_table, sizeof(func_info));
        free_array(gst.entries, sizeof(var_info));

        char **data = (char **)idents.data;
        for (size_t i = 0; i < idents.size; i++) {
                free(data[i]);
        }

        free_array(&idents, sizeof(char *));

        return EXIT_SUCCESS;
}

static ast_node *syntax_error(ast_node *root)
{
        assert(root);
        fprintf(stderr, ascii(red, "Syntax error:\n"));
        save_ast_tree(stderr, root);
        $(dump_tree(root);)
        fprintf(stderr, "\n");
        return root;
}

static ast_node *success(ast_node *root)
{
        return nullptr;
}


static int keyword(ast_node *node)
{
        assert(node);

        if (node->type == AST_NODE_KEYWORD)
                return ast_keyword(node);

        return 0;
}

/*
static double *number(ast_node *node)
{
        assert(node);

        if (node->type == AST_NODE_NUMBER)
                return &node->data.number;

        return 0;
}

static const char *ident(ast_node *node)
{
        assert(node);

        if (node->type == AST_NODE_IDENT)
                return ast_ident(node);

        return nullptr;
}
*/

static func_info *find_function(ast_node *function,
                                array *const func_table)
{
        assert(function);
        assert(func_table);

        const char *ident = ast_ident(function);
        func_info *funcs = (func_info *)(func_table->data);
        for (size_t i = 0; i < func_table->size; i++) {
                if (funcs[i].ident == ident)
                        return &funcs[i];
        }

        return nullptr;
}

static const char *create_variable(ast_node *variable,
                                   scope_table *const local_table,       
                                   scope_table *const global_table)
{
        assert(local_table);
        assert(global_table);
        assert(variable);

        var_info *var = nullptr;

        var = scope_table_find(global_table, variable);
        if (var)
                return nullptr;

        var = scope_table_find(local_table, variable);
        if (var)
                return nullptr;

        var = scope_table_add(local_table, variable);
        if (var)
                return find_variable(variable, local_table, global_table);

        return nullptr;
}

static const char *get_variable(ast_node *variable,
                                scope_table *const local_table,       
                                scope_table *const global_table)
{
        assert(local_table);
        assert(global_table);
        assert(variable);

        var_info *var = nullptr;

        size_t shift = 0;
        if (variable->right && variable->right->type == AST_NODE_NUMBER)
                shift += ast_number(variable->right);

        var = scope_table_find(global_table, variable);
        if (var)
                return global_variable(var, shift);

        var = scope_table_find(local_table, variable);
        if (var)
                return local_variable(var, shift);

        var = scope_table_add(local_table, variable);
        if (var)
                return find_variable(variable, local_table, global_table);

        return nullptr;
}

static const char *find_variable(ast_node *variable,
                                 scope_table *const local_table,       
                                 scope_table *const global_table)
{
        assert(local_table);
        assert(global_table);
        assert(variable);

        var_info *var = nullptr;

        size_t shift = 0;
        if (variable->right && variable->right->type == AST_NODE_NUMBER)
                shift += ast_number(variable->right);

        var = scope_table_find(global_table, variable);
        if (var)
                return global_variable(var, shift);

        var = scope_table_find(local_table, variable);
        if (var)
                return local_variable(var, shift);

        return nullptr;
}

#define CMD(name, code, str, hash)                   \
        static inline void name(const char *arg)     \
        {                                            \
                if (arg)                             \
                        printf("%s %s\n", str, arg); \
                else                                 \
                        printf("%s\n", str);         \
        }

#include <commands>
#undef CMD

static inline void LABEL(const char *arg)
{
        assert(arg);
        printf("%s:\n", arg);
}

static inline void WRITE(const char *arg)
{
        assert(arg);
        printf("%s\n", arg);
}

static inline const char *id(const char *name, void *id)
{
        assert(name);
        snprintf(BUFFER, BUFSIZE, "%s.%lx", name, id);
        return BUFFER;
}

static inline const char *number_str(double num)
{
        snprintf(BUFFER, BUFSIZE, "%lg", num);
        return BUFFER;
}

static inline const char *memory(const char *reg, ptrdiff_t shift)
{
        snprintf(BUFFER, BUFSIZE, "[%s + %lu]", reg, shift);
        return BUFFER;
}

static inline const char *global_variable(var_info *var, ptrdiff_t shift)
{
        assert(var);
        snprintf(BUFFER, BUFSIZE, "[%s + %lu]", GLOBAL_REG, var->shift + shift);
        return BUFFER;
}

static inline const char *local_variable(var_info *var, ptrdiff_t shift)
{
        assert(var);
        snprintf(BUFFER, BUFSIZE, "[%s + %lu]", LOCAL_REG, var->shift + shift);
        return BUFFER;
}

static void dump_array_function(void *item)
{
        assert(item);
        func_info *func = (func_info *)item;
        if (!func->node)
                return;

        fprintf(logs, "%s(", func->ident);
        ast_node *param = func->node->right;
        if (param) {
                while (param->left) {
                        fprintf(logs, "%s, ", ast_ident(param->right));
                        param = param->left;
                }

                fprintf(logs, "%s", ast_ident(param->right));
        }

        fprintf(logs, ")", func->n_params);
}
