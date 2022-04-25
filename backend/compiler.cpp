#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <array.h>
#include <iommap.h>
#include <stack.h>
#include <assert.h>
#include <logs.h>

#include <ast/tree.h>
#include <ast/keyword.h>

#include <backend/backend.h>
#include <backend/elf64.h>

static int       keyword(ast_node *node);
static double    *number(ast_node *node);
static const char *ident(ast_node *node);

static ast_node *success     (ast_node *root);
static ast_node *syntax_error(ast_node *root);

static ast_node *dump_code (ast_node *root);


#define require(__node, __ast_type)                             \
        do {                                                    \
        if (__node && keyword(__node) != __ast_type)            \
                return syntax_error(root);                      \
        } while (0)

#define require_ident(__node)                                   \
        do {                                                    \
        if (!__node || __node->type != AST_NODE_IDENT)          \
                return syntax_error(root);                      \
        } while (0)

#define require_number(__node)                                  \
        do {                                                    \
        if (!__node || __node->type != AST_NODE_NUMBER)         \
                return syntax_error(root);                      \
        } while (0)

static ac_symbol *find_symbol(stack *symtabs, const char *ident);

static void encode(ac_virtual_memory *vm, void *instruction, size_t size);

/*
static ast_node *compile_return(ast_node *root, symbol_table *table);
static ast_node *compile_define(ast_node *root, symbol_table *table);
static ast_node *compile_assign(ast_node *root, symbol_table *table);
static ast_node *compile_expr  (ast_node *root, symbol_table *table);
static ast_node *compile_if    (ast_node *root, symbol_table *table);
static ast_node *compile_while (ast_node *root, symbol_table *table);
static ast_node *compile_call  (ast_node *root, symbol_table *table);
*/

static void dump_symtab(stack *symtab);

static ast_node *declare_functions(ast_node *root, stack *symtabs);

static ast_node *compile_stmt  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_define(ast_node *root, stack *symtabs, ac_virtual_memory *vm);

ast_node *compile_tree(ast_node *tree, elf64_section *secs, elf64_symbol *syms)
{
        assert(tree);
        assert(secs);
        assert(syms);
        
        ast_node *error = nullptr;

        ac_virtual_memory vm = {};
        vm.secs = secs;
        vm.syms = syms;

        stack symtab = {};
        construct_stack(&symtab);

        /* Init global scope table to declare functions */
        array global_scope = {};
        push_stack(&symtab, &global_scope);

        declare_functions(tree, &symtab);
        dump_symtab(&symtab);

        compile_define(tree->right, &symtab, &vm);
        //        compile_stmt(tree, &symtabs, &vm);
        
        free_array(&global_scope, sizeof(ac_symbol));
        destruct_stack(&symtab);
        
        return error;
}

static ast_node *compile_stmt(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        return nullptr;
}

static ast_node *compile_define(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);
$$
        ast_node *error = nullptr;
$$
        require(root, AST_DEFINE);
$$
        
        ast_node *function = root->left;
        require(function, AST_FUNC);
$$

        ast_node *name = function->left;
        require_ident(name);   
$$
$$
        ac_symbol *func_sym = find_symbol(symtabs, ast_ident(name));
        if (!func_sym)
                return syntax_error(root);
$$
        func_sym->offset = vm->secs[SEC_TEXT].size;
$$

        array symtab = {};
        push_stack(symtabs, &symtab);
$$

        ptrdiff_t n_params = func_sym->info;
        
$$
        ast_node *param = function->right;
        while (param) {

                require_ident(param->right);
                ac_symbol *param_sym = find_symbol(symtabs, ast_ident(param->right));
                if (param_sym)
                        return syntax_error(root);
                
                ac_symbol symbol = {
                        .type   = AC_SYM_VAR,
                        .vis    = AC_VIS_LOCAL,
                        .ident  = ast_ident(param->right),
                        .node   = name,
                        .offset = -8 * n_params,
                        .info   = 8,               
                };
                        
                array_push((array *)top_stack(symtabs), &symbol, sizeof(ac_symbol));

                n_params--;                            
                param = param->left;
                require(param, AST_PARAM);
        }
$$

        if (n_params)
                return syntax_error(root);

        /* push %rbp */
        struct __attribute__((packed)) {
                const ubyte opcode = 0x50 + IE64_RBP;
        } push_rbp;
        
        encode(vm, &push_rbp, sizeof(push_rbp));

        /* mov %rsp, %rbp */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                
                const ie64_modrm modrm = { 
                        .rm = IE64_RSP, .reg = IE64_RBP, .mod = 0b11, 
                };
                
        } mov_rsp_rbp;
        
        encode(vm, &mov_rsp_rbp, sizeof(mov_rsp_rbp));

        error = compile_stmt(root->right, symtabs, vm);
        if (error)
                return error;

        
        /* mov %top, %rax */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                
                ie64_modrm modrm = { 
                        .rm = IE64_RSP, .reg = 0b000, .mod = 0b11, 
                };
        } init_ret;
        init_ret.modrm.reg = vm->reg.stack--;
        encode(vm, &init_ret, sizeof(init_ret));

        /* pop %rbp */
        struct __attribute__((packed)) {
                const ubyte opcode = 0x58 + IE64_RBP;
        } pop_rbp;
        
        encode(vm, &pop_rbp, sizeof(pop_rbp));

        /* ret */
        struct __attribute__((packed)) {
                ubyte opcode = 0xc3;     
        } ret;
        
        encode(vm, &ret, sizeof(ret));
        
        pop_stack(symtabs);
        free_array(&symtab, sizeof(ac_symbol));
        
        return success(root);
}

static ast_node *declare_functions(ast_node *root, stack *symtabs)
{
        assert(root);
        assert(symtabs);
        
        ast_node *error = nullptr;
$$
        require(root, AST_STMT);
$$
        if (root->left) {
                error = declare_functions(root->left, symtabs);
                if (error)
                        return error;                
        }
$$
        ast_node *define = root->right;
        if (keyword(define) != AST_DEFINE)
                return success(root);
$$
        ast_node *function = define->left;
        require(function, AST_FUNC);
$$
        ast_node *name = function->left;
        require_ident(name);
$$        
        ac_symbol *exist = find_symbol(symtabs, ast_ident(name));
        if (exist)
                return syntax_error(name);
$$
        ac_symbol symbol = {
                .type  = AC_SYM_FUNC,
                .vis   = AC_VIS_GLOBAL,
                .ident = ast_ident(name),
                .node  = define,
                .info  = 0              
        };
$$
        ast_node *param = function->right;
        while (param) {
                symbol.info++;
                param = param->left;
                require(param, AST_PARAM);
        }
$$
        array_push((array *)bottom_stack(symtabs), &symbol, sizeof(ac_symbol));
        return success(root);
}

static int keyword(ast_node *node)
{
        assert(node);

        if (node->type == AST_NODE_KEYWORD)
                return ast_keyword(node);

        return 0;
}

static ac_symbol *find_symbol(stack *symtabs, const char *ident)
{
        assert(ident);

        for (size_t i = 0; i < symtabs->size; i++) {
        
                array *symtab = (array *)symtabs->items[i];
                ac_symbol *symbols = (ac_symbol *)symtab->data;

                for (size_t j = 0; j < symtab->size; j++) {
                        if (ident == symbols[j].ident)
                                return &symbols[j];                                        
                }
        } 

        return nullptr;       
}

static ast_node *dump_code(ast_node *root)
{
        assert(root);
        fprintf(stderr, ";");
        save_ast_tree(stderr, root);
        fprintf(stderr, "\n");
        return root;
}

static ast_node *syntax_error(ast_node *root)
{
        assert(root);
        fprintf(stderr, ascii(RED, "Syntax error:\n"));
        save_ast_tree(stderr, root);
        $(dump_tree(root);)
        fprintf(stderr, "\n");
        return root;
}

static ast_node *success(ast_node *root)
{
        return nullptr;
}

static void encode(ac_virtual_memory *vm, void *instruction, size_t size)
{
        assert(vm);
        assert(instruction);
        section_memcpy(vm->secs + SEC_TEXT, instruction, size);
}

static void dump_symtab(stack *symtabs) 
{
        assert(symtabs);
$$        
        fprintf(logs, html(BLUE, "---------------------------------\n"));
        fprintf(logs, html(BLUE, "Assert compiler symbol table dump\n"));
        fprintf(logs, html(BLUE, "---------------------------------\n"));

        for (size_t i = 0; i < symtabs->size; i++) {

                fprintf(logs, html(BLUE, "symtab[%lu]\n"), i);

                array *symtab = (array *)symtabs->items[i];
                ac_symbol *symbols = (ac_symbol *)symtab->data;

                for (size_t j = 0; j < symtab->size; j++) {
                        fprintf(logs, html(GREEN,"\tsymbol[%lu]:\n"), j);                                       
                        fprintf(logs, "\t\ttype   = %d\n", symbols[j].type);                                       
                        fprintf(logs, "\t\tvis    = %d\n", symbols[j].vis);                                       
                        fprintf(logs, "\t\tident  = '%s'\n", symbols[j].ident);                                       
                        fprintf(logs, "\t\tnode   = %p\n", symbols[j].node);                                       
                        fprintf(logs, "\t\toffset = %lx\n", symbols[j].offset);                                       
                        fprintf(logs, "\t\tinfo   = %lu\n\n", symbols[j].info);                                       
                }
                
                fprintf(logs, html(BLUE, "---------------------------------\n\n"));
        } 
}
