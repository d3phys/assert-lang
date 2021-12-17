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

struct func_info {
        ast_node    *node = nullptr;
        const char *ident = 0;
        size_t n_params   = 0;
};

static const size_t BUFSIZE = 128;
static char BUFFER[BUFSIZE] = {0};

static const char *const GLOBAL_REG = "cx";
static const char *const LOCAL_REG  = "bx";

static int       keyword(ast_node *node);
static double    *number(ast_node *node);
static const char *ident(ast_node *node);

static ast_node *success(ast_node *root);
static ast_node *syntax_error(ast_node *root);

static void dump_array_function(void *item);

static inline void LABEL(const char *arg);

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

static const char *create_variable(ast_node *variable,
                                   scope_table *const local_table,       
                                   scope_table *const global_table);

static const char *get_variable(ast_node *variable,
                                scope_table *const local_table,       
                                scope_table *const global_table);

static const char *find_variable(ast_node *variable,
                                 scope_table *const local_table,       
                                 scope_table *const global_table);

static ast_node *declare_function(ast_node *root, array *const func_table);


/*
static ast_node *global_assign_rule(ast_node *root, array *const vars_table, 
                                                    array *const func_table)
{
        assert(root);
        assert(vars_table);
        assert(func_table);

        ast_node *error = 0;
        if (keyword(root) != AST_ASSIGN)
                return syntax_error(root);
                        
        if (!root->left)
                return syntax_error(root);

        error = expression_rule(root->left, vars_table, func_table);
        if (error)
                return error;

        error = vars_table_add(vars_table, left->SCOPE_GLOBAL);
        if (error)
                return error;


        return success(root);
}
*/

static ast_node *main_brunch_assign(ast_node *root, 
                                    array *const func_table,
                                    scope_table *const global_table)
{
        assert(root);
        assert(func_table);
        ast_node *error = nullptr;

        require(root, AST_STMT);

        if (root->left) {
                error = main_brunch_assign(root->left, func_table, global_table);
                if (error)
                        return error;
        }

        if (keyword(root->right) != AST_ASSIGN)
                return success(root); 

        error = assign_rule(root, func_table, global_table, global_table);
        if (error)
                return error;

        require_ident(root->right);
        const char *ident = create_variable(root->right, global_table, global_table);
        if (!ident)
                return syntax_error(root);

        POP(ident);
        
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

        scope_table st = {0};
        array entries = {0};
        st.entries = &entries;

        PUSH("Hello");
        PUSH(create_variable(rt->left->left->left->right->left, &st, &st));
        POP(find_variable(rt->left->left->left->right->left, &st, &st));
        PUSH(create_variable(rt->left->left->right->left, &st, &st));
        dump_array(st.entries, sizeof(var_info), dump_array_var_info);

        array func_table = {0};
        main_brunch_functions(rt, &func_table);
        //declare_func(rt->right, &func_table);
        dump_array(&func_table, sizeof(func_info), dump_array_function);
        free_array(&func_table, sizeof(func_info));

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
        fprintf(stderr, "Syntax error!!!!");
        $(dump_tree(root);)
        return root;
}

static ast_node *success(ast_node *root)
{
        fprintf(stderr, "Success: root");
        return nullptr;
}

/*
static ast_node *expression_rule(ast_node *root)
{
        return nullptr;
}

static ast_node *condition_rule(ast_node *root)
{
        assert(root);
        ast_node *error = nullptr;

        if (!root->left || !root->right)
                return syntax_error(root);

        error = expression_rule(root->left);
        if (error)
                return error;

        error = expression_rule(root->right);
        if (error)
                return error;

        switch (keyword(root)) {
        case AST_EQUAL:
                EQ();
                break;
        case AST_GEQUAL:
                AEQ();
                break;
        case AST_LEQUAL:
                BEQ();
                break;
        case AST_GREAT:
                AB();
                break;
        case AST_LOW:
                BE();
                break;
        default:
                return syntax_error(root);
        }

        return success(root);
}

static ast_node *while_rule(ast_node *root)
{
        assert(root);
        ast_node *error = nullptr;

        //write(label("while:", root));

        error = condition_rule(root->left);
        if (error)
                return error;

        PUSH("0");
        //JE(U("while_end", root));
        //cmd(JE, label("while_end", root));

        error = statement_rule(root->right->left);
        if (error)
                return error;

        //write(label("while_end:", root));
        return success(root);
}

static void print_unique(const char *str, void *ptr)
{
        assert(str);
        assert(ptr);

        printf("%p%s", ptr, str);
}

static void label(const char *str, void *ptr)
{
        assert(str);
        assert(ptr);

        print_unique(str, ptr);
        printf(":\n");
}

static ast_node *if_rule(ast_node *root)
{
        static const char IF_FAIL[] = "if_fail";
        static const char IF_END [] = "if_end";

        assert(root);
        ast_node *error = nullptr;

        error = condition_rule(root->left);
        if (error)
                return error;

        if (!root->right->left)
                return syntax_error(root);

        PUSH("0");
        //LABEL(U("if_start", root));
        //cmd(PUSH, "0");
        //cmd(JE, label(IF_FAIL, root));

        //je(label("end_if"));


        error = statement_rule(root->right->left);
        if (error)
                return error;

        if (root->right->right) {
                //cmd(JMP, label(IF_END, root));
                //write(label(IF_FAIL, root));

                error = statement_rule(root->right->left);
                if (error)
                        return error;

                //write(label(IF_END, root));
        } else {
                printf("hello");
                //write(label(IF_FAIL, root));
        }

        return success(root);
}

static ast_node *statement_rule(ast_node *root)
{
        assert(root);
        ast_node *error = nullptr;

        if (keyword(root) != AST_STMT)
                return syntax_error(root);

        if (root->left) {
                error = statement_rule(root->left);
                if (error)
                        return error;
        }

        switch (keyword(root->right)) {
        case AST_WHILE:
                return while_rule(root->right);
        case AST_CALL:
                return call_rule(root->right);
        case AST_IF:
                return if_rule(root->right);
        case AST_RETURN:
                return return_rule(root->right);
        default:
                return syntax_error(root);
        }

        return success(root);
}

*/


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

        var = scope_table_find(global_table, variable);
        if (var)
                return global_variable(var, shift);

        var = scope_table_find(local_table, variable);
        if (var)
                return local_variable(var, shift);

        var = scope_table_add(local_table, variable);
        if (var)
                return find_variable(variable, local_table, global_table, var->shift);

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

static inline const char *id(const char *name, void *id)
{
        assert(name);
        snprintf(BUFFER, BUFSIZE, "%s.%lx", name, id);
        return BUFFER;
}

static inline const char *number(double num)
{
        snprintf(BUFFER, BUFSIZE, "%lg", num);
        return BUFFER;
}
static inline const char *global_variable(var_info *var)
{
        assert(var);
        ptrdiff_t shift = var->shift;
        if (var->right)
                shift += ast_number(var->node);

        snprintf(BUFFER, BUFSIZE, "[%s + %lu]", GLOBAL_REG, var->shift + shift);
        return BUFFER;
}

static inline const char *local_variable(var_info *var)
{
        assert(var);
        ptrdiff_t shift = var->shift;
        if (var->right)
                shift += ast_number(var->node);

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
