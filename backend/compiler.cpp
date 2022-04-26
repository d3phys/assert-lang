#include <elf.h>
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

static ast_node *compile_stdcall(ast_node *root, stack *symtabs, ac_virtual_memory *vm, const int sym_index);

static ast_node *compile_return(ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_assign(ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_expr  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_if    (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_while (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_call  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_stmt  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_define(ast_node *root, stack *symtabs, ac_virtual_memory *vm);

static ast_node *compile_load_num(ast_node *root, stack *symtabs, ac_virtual_memory *vm);

static ast_node *compile_sym_load (ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym);
static ast_node *compile_sym_store(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym);

static ast_node *compile_mov_global(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym, bool store);
static ast_node *compile_mov_local (ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym, bool store);

static void dump_symtab(stack *symtabs); 

static ast_node *declare_functions(ast_node *root, stack *symtabs);

//static ast_node *compile_expr  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);

ast_node *compile_tree(ast_node *tree, elf64_section *secs, elf64_symbol *syms)
{
        assert(tree);
        assert(secs);
        assert(syms);
        
        ast_node *error = nullptr;
$$

        ac_virtual_memory vm = {};
        vm.secs = secs;
        vm.syms = syms;
$$

        stack symtab = {};
        construct_stack(&symtab);
$$

        /* Init global scope table to declare functions */
        array global_scope = {};
        push_stack(&symtab, &global_scope);
$$

        declare_functions(tree, &symtab);
        dump_symtab(&symtab);
$$

        compile_define(tree->right, &symtab, &vm);
        //        compile_stmt(tree, &symtabs, &vm);
$$

        free_array(&global_scope, sizeof(ac_symbol));
        destruct_stack(&symtab);
$$
        
        return error;
}

inline static int is_global_scope(stack *symtabs) 
{
$$
        fprintf(logs, html(GREEN, "IS_GLOBAL: %d\n"), symtabs->size < 2);
        return symtabs->size < 2;
}

static ast_node *compile_assign(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs); 
        ast_node *error = nullptr;
$$
        require(root, AST_ASSIGN);        
$$
        error = compile_expr(root->right, symtabs, vm);
        if (error)
                return error;
$$
        require_ident(root->left);
$$

        ac_symbol *sym = find_symbol(symtabs, ast_ident(root->left));
        if (sym)
                return compile_sym_store(root->left, symtabs, vm, sym);
$$

        int type = AC_SYM_VAR;
        int vis  = AC_VIS_LOCAL;
        int sec  = SEC_NULL;
$$

        ptrdiff_t offset = 0x0;
        size_t size = 8;
        ast_node *cap = root->left->right;
        if (cap) {
                require_number(cap);
                size = 8 * (ast_number(cap) + 1);
        }
$$
        if (is_global_scope(symtabs)) {
                vis = AC_VIS_GLOBAL;
                sec = SEC_BSS;       
        }

$$
        ac_symbol symbol = {
                .type   = type,
                .vis    = vis,
                .ident  = ast_ident(root->left),
                .node   = root,
                .offset = offset,
                .info   = size,               
        };
$$
        symbol.addend = vm->secs[sec].size;
        vm->secs[sec].size += size;

        array_push((array *)top_stack(symtabs), &symbol, sizeof(ac_symbol));
$$
$       (dump_symtab(symtabs);)

        return compile_sym_store(root->left, symtabs, vm, &symbol);
}

static ast_node *compile_expr(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs); 
        ast_node *error = nullptr;
        /*
        if (keyword(root) == AST_CALL)
                return compile_call(root, symtabs, vm);
        */
$$
        if (keyword(root)) {
                if (root->left) {
                        error = compile_expr(root->left, symtabs, vm);
                        if (error)
                                return error;
                }
$$
                if (root->right) {
                        error = compile_expr(root->right, symtabs, vm);
                        if (error)
                                return error;
                }
        }        

        ac_symbol *sym = nullptr;
        switch (root->type) {
        case AST_NODE_NUMBER:
$$              return compile_load_num(root, symtabs, vm);
        case AST_NODE_IDENT:
                fprintf(stderr, "ast_ident:%s\n", ast_ident(root));
$$              sym = find_symbol(symtabs, ast_ident(root));
                if (!sym)
                        return syntax_error(root);
                        
$$              return compile_sym_load(root, symtabs, vm, sym);
        default:
                break;
        }
$$
        if (keyword(root) == AST_ADD) {
$$
                struct __attribute__((packed)) {
                        const ubyte rex    = 0b01001101;
                        const ubyte opcode = 0x01;
                        ie64_modrm modrm   = { .rm   = 0b000, .reg = 0b000, .mod = 0b11 };
                } add;
$$
                add.modrm.reg = vm->reg.stack;
                vm->reg.stack--;
                add.modrm.rm = vm->reg.stack;
                fprintf(stderr, "AST_ADD: size %lu\n rex %x\nopcode %x\n", sizeof(add), add.rex, add.opcode);
                encode(vm, &add, sizeof(add));
                return success(root);        
        }

        switch (keyword(root)) {
        case AST_ADD:
/*

        case AST_SUB:
                SUB();
                return success(root);
        case AST_MUL:
                MUL();
                return success(root);
        case AST_DIV:
                DIV();
                return success(root);
        case AST_EQUAL:
                EQ();
                return success(root);
        case AST_NEQUAL:
                NEQ();
                return success(root);
        case AST_GREAT:
                AB();
                return success(root);
        case AST_LOW:
                BE();
                return success(root);
        case AST_GEQUAL:
                AEQ();
                return success(root);
        case AST_LEQUAL:
                BEQ();
                return success(root);
        case AST_NOT:
                NOT();
                return success(root);
        case AST_AND:
                AND();
                return success(root);
        case AST_OR:
                OR();
                return success(root);
*/
        case AST_POW:
$$
                return compile_stdcall(root, symtabs, vm, SYM_POW);
        case AST_SIN:
$$
                return compile_stdcall(root, symtabs, vm, SYM_SIN);
        case AST_COS:
$$
                return compile_stdcall(root, symtabs, vm, SYM_COS);
        case AST_IN:
$$
                return compile_stdcall(root, symtabs, vm, SYM_SCAN);
        default:
$$
                break;
        }
$$
        return syntax_error(root);
        
}

static ast_node *compile_load_num(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);   
$$

        require_number(root);
$$

        /* movabs $imm64, %++top */
        struct __attribute__((packed)) {
                const ubyte rex = 0b01001001;
                ubyte opcode    = 0xb8;  
                imm64 imm       = 0;   
        } movabs;
$$

        movabs.imm = (imm64)ast_number(root);
$$

        vm->reg.stack++;
        movabs.opcode += vm->reg.stack;
$$

        encode(vm, &movabs, sizeof(movabs));
$$

        return success(root);
}

static ast_node *compile_mov_local(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym, bool store)
{
        assert(vm);
        assert(sym);
        assert(root);
$$

        ast_node *error = nullptr;
        require_ident(root);
$$
        
        if (root->right) {
$$
        
                /* mov %reg <- [%rbp + %reg * 8 + addend] */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0b01001110;
                        ubyte opcode       = 0x00;
                        ie64_modrm modrm   = { .rm   = 0b100, .reg = 0b000, .mod = 0b10 };
                        ie64_sib sib       = { .base = IE64_RBP, .index = 0b000, .scale = 0b11 };  
                        imm32 imm          = 0;   
                } mov;
$$

                if (store) {
                        mov.opcode = 0x89;
$$                  
                        mov.sib.index = vm->reg.stack;
                        vm->reg.stack--;
$$                      
                        mov.modrm.reg = vm->reg.stack;               
                        vm->reg.stack--;                        
                } else {
                        mov.opcode = 0x89;

                        mov.sib.index = vm->reg.stack;
                        mov.modrm.reg = vm->reg.stack;               
                }
$$

                mov.imm = -sym->addend - 0x8;
$$
                encode(vm, &mov, sizeof(mov));
                return success(root);
        } 

        /* mov %reg <- [%rbp + addend] */
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4c; //0b01001100;
                ubyte opcode       = 0x00;
                ie64_modrm modrm   = { .rm = IE64_RBP, .reg = 0b000, .mod = 0b10 };
                imm32 imm          = 0;   
        } mov;         
$$

        if (store) {
$$
                mov.opcode = 0x89;
                mov.modrm.reg = vm->reg.stack;
                vm->reg.stack--; 
        } else {
$$
                mov.opcode = 0x8b;
                vm->reg.stack++; 
                mov.modrm.reg = vm->reg.stack;
        }
$$
        fprintf(stderr, "SYM: %s\n", sym->ident);
        mov.imm = -sym->addend - 0x8;
$$
        encode(vm, &mov, sizeof(mov));
        return success(root);
}

static ast_node *compile_mov_global(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym, bool store)
{
        assert(vm);
        assert(sym);
        assert(root);

        ast_node *error = nullptr;
        require_ident(root);
$$

        Elf64_Rela rela = {
                .r_offset = vm->secs[SEC_TEXT].size, 
                .r_info   = ELF64_R_INFO(SEC_BSS, R_X86_64_32S), 
                .r_addend = sym->addend,
        }; 
$$    
        if (root->right) {

                /* mov %reg <- section[addend] */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0b01001110;
                        ubyte opcode = 0x00;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b10 };
                        ie64_sib sib       = { .base = 0b101, .index = 0b000, .scale = 0b11 };
                        imm32 imm          = 0;   
                } mov;          
$$
                rela.r_offset += 0x04;

                if (store) {
                        mov.opcode = 0x89;
                        mov.sib.index = vm->reg.stack;
                        vm->reg.stack--;
$$

                        mov.modrm.reg = vm->reg.stack;
                        vm->reg.stack--;                
                } else {
                        mov.opcode = 0x8b;
                        mov.sib.index = vm->reg.stack;
                        mov.modrm.reg = vm->reg.stack;                                        
                }
$$     
                encode(vm, &mov, sizeof(mov));

        } else {
$$
                
                /* mov %reg <- section[%reg + addend] */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0b01001100;
                        ubyte opcode       = 0x00;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b10 };
                        imm32 imm          = 0;   
                } mov;          
$$
                rela.r_offset += 0x03;

                if (store) {
                        mov.opcode = 0x89;
                        mov.modrm.reg = vm->reg.stack;
                        vm->reg.stack--;
$$
                         
                } else {
                        mov.opcode = 0x8b; 
                        vm->reg.stack++;
                        mov.modrm.reg = vm->reg.stack;
                }
$$     
                encode(vm, &mov, sizeof(mov));
        }                                              
$$
        section_memcpy(vm->secs + SEC_RELA_TEXT, &rela, sizeof(Elf64_Rela));      
        return success(root);       
}


static ast_node *compile_sym_load(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym)
{
        assert(vm);
        assert(root);
        assert(symtabs);

        require_ident(root);
$$

        if (root->right) {
                ast_node *error = compile_expr(root->right, symtabs, vm);
                if (error)
                        return error;
        }
$$

        if (sym->vis == AC_VIS_LOCAL)
                return compile_mov_local (root, symtabs, vm, sym, false);
        else if (sym->vis == AC_VIS_GLOBAL)
                return compile_mov_global(root, symtabs, vm, sym, false);
$$
        
        return syntax_error(root);
}

static ast_node *compile_sym_store(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym)
{
        assert(vm);
        assert(root);
        assert(symtabs);
        assert(sym);
$$

        require_ident(root);
$$

        if (root->right) {
                ast_node *error = compile_expr(root->right, symtabs, vm);
                if (error)
                        return error;
        }
$$

        if (sym->vis == AC_VIS_LOCAL)
                return compile_mov_local (root, symtabs, vm, sym, true);
        else if (sym->vis == AC_VIS_GLOBAL)
                return compile_mov_global(root, symtabs, vm, sym, true);
$$
        
        return syntax_error(root);
}


static ast_node *compile_stdcall(ast_node *root, stack *symtabs, ac_virtual_memory *vm, const int sym_index)
{
        assert(vm);
        assert(root);
        assert(symtabs); 
        ast_node *error = nullptr;
$$
        if (!root->right)
                return syntax_error(root);
$$                
        error = compile_expr(root->right, symtabs, vm);
        if (error)
                return error;
$$
        elf64_symbol *sym = vm->syms + sym_index;
        assert(sym);
$$
        /* push arguments */
        for (size_t i = 0; i < sym->info; i++) {
                /* push %top-- */
                struct __attribute__((packed)) {
                        ubyte opcode = 0x50;
                } push_arg;           
$$
                push_arg.opcode += vm->reg.stack;
                vm->reg.stack--;
                
                encode(vm, &push_arg, sizeof(push_arg));        
        }
$$
        /* call stdfunc */
        struct __attribute__((packed)) {
                const ubyte opcode = 0xe8;
                imm32 imm          = 0x00;
        } call; 
$$
        Elf64_Rela rela = {
                .r_offset = vm->secs[SEC_TEXT].size + 0x01, 
                .r_info   = ELF64_R_INFO(sym_index, R_X86_64_PC32), 
                .r_addend = -0x04,
        }; 
$$     
        section_memcpy(vm->secs + SEC_RELA_TEXT, &rela, sizeof(Elf64_Rela));      
        encode(vm, &call, sizeof(call));
$$
        /* mov %rax, %++top */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0b01001001;
                const ubyte opcode = 0x89;
                ie64_modrm modrm = { .rm = 0b000, .reg = IE64_RAX, .mod = 0b11 };         
        } mov_rax_top;      
$$        
        vm->reg.stack++;
        mov_rax_top.modrm.rm = vm->reg.stack;
$$
        encode(vm, &mov_rax_top, sizeof(mov_rax_top));
$$
        return success(root);
}

static ast_node *compile_return(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);        

$$
        ast_node *error = nullptr;
$$
        
        require(root, AST_RETURN);
        if (!root->right)
                return syntax_error(root);
$$
                
        error = compile_expr(root->right, symtabs, vm);
        if (error)
                return error;
$$

        /* mov %top, %rax */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0b01001100;
                const ubyte opcode = 0x89;        
                ie64_modrm modrm = { .rm = IE64_RAX, .reg = 0b000, .mod = 0b11, };
        
        } init_ret;
        
$$
        init_ret.modrm.reg = vm->reg.stack;
        vm->reg.stack--;
$$
        
        encode(vm, &init_ret, sizeof(init_ret));
$$

        /* pop %rbp */
        struct __attribute__((packed)) {
                const ubyte opcode = 0x58 + IE64_RBP;
        } pop_rbp;
$$
        
        encode(vm, &pop_rbp, sizeof(pop_rbp));
$$

        /* ret */
        struct __attribute__((packed)) {
                ubyte opcode = 0xc3;     
        } ret;
        
        encode(vm, &ret, sizeof(ret));
$$

        return success(root);
}

static ast_node *compile_stmt(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);

        ast_node *error = nullptr;
$$
        require(root, AST_STMT);

        if (root->left) {
$$
                error = compile_stmt(root->left, symtabs, vm);
$$
                if (error)
                        return error;
        }

        if (!root->right)
                return syntax_error(root);
        if (!keyword(root))
                return syntax_error(root);
$$
        switch (ast_keyword(root->right)) {
        case AST_ASSIGN:
$$
                return compile_assign(root->right, symtabs, vm);

        case AST_DEFINE:
                return compile_define(root->right, symtabs, vm);
/*
        case AST_IF:
$$
                return compile_if(root->right, symtabs, vm);
        case AST_WHILE:
$$
                return compile_while(root->right, symtabs, vm);
*/
        case AST_SHOW:
                fprintf(stderr, "AST_SHOW is not supported\n");
                return syntax_error(root);
/*        case AST_CALL:
$$
                return compile_call(root->right, symtabs, vm);
*/        case AST_OUT:
$$
                return compile_stdcall(root->right, symtabs, vm, SYM_PRINT);
        case AST_RETURN:
$$
                return compile_return(root->right, symtabs, vm);
        default:
$$
                return syntax_error(root);
        }
$$
        return success(root);
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
$$

                require_ident(param->right);
                ac_symbol *param_sym = find_symbol(symtabs, ast_ident(param->right));
                if (param_sym)
                        return syntax_error(root);
                
                ac_symbol symbol = {
                        .type   = AC_SYM_VAR,
                        .vis    = AC_VIS_LOCAL,
                        .ident  = ast_ident(param->right),
                        .node   = name,
                        .offset = 8 * (n_params + 1),
                        .info   = 8,               
                };
$$
                        
                array_push((array *)top_stack(symtabs), &symbol, sizeof(ac_symbol));
$$

                n_params--;                            
                param = param->left;
                require(param, AST_PARAM);
        }
$$
        dump_symtab(symtabs);
        if (n_params)
                return syntax_error(root);
$$


        /* push %rbp */
        struct __attribute__((packed)) {
                const ubyte opcode = 0x50 + IE64_RBP;
        } push_rbp;
$$
        
        encode(vm, &push_rbp, sizeof(push_rbp));
$$

        /* mov %rsp, %rbp */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                const ie64_modrm modrm = { .rm = IE64_RSP, .reg = IE64_RBP, .mod = 0b11 };         
        } mov_rsp_rbp;
$$
        
        encode(vm, &mov_rsp_rbp, sizeof(mov_rsp_rbp));
$$

        error = compile_stmt(root->right, symtabs, vm);
        if (error)
                return error;
$$

        pop_stack(symtabs);
        free_array(&symtab, sizeof(ac_symbol));
$$
        
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
                        fprintf(logs, "\t\toffset = 0x%lx (%ld)\n", symbols[j].offset, symbols[j].offset);                                       
                        fprintf(logs, "\t\tinfo   = %lu\n", symbols[j].info);                                       
                        fprintf(logs, "\t\taddend = 0x%lx (%ld)\n\n", symbols[j].addend, symbols[j].addend);                                       
                }
                
                fprintf(logs, html(BLUE, "---------------------------------\n\n"));
        } 
}
