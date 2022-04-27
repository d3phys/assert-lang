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

static char *encode(ac_virtual_memory *vm, void *instruction, size_t size);

static ast_node *compile_stdcall(ast_node *root, stack *symtabs, ac_virtual_memory *vm, const int sym_index);

static ast_node *compile_return(ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_assign(ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_expr  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_if    (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_while (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_call  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_stmt  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_define(ast_node *root, stack *symtabs, ac_virtual_memory *vm);

static void compile_start(stack *symtabs, ac_virtual_memory *vm);

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
        array func_scope = {};
        push_stack(&symtab, &func_scope);
$$

        declare_functions(tree, &symtab);

        ac_symbol *functions = (ac_symbol *)func_scope.data;
        for (size_t i = 0; i < func_scope.size; i++) {
                if (!strncmp(functions[i].ident, "main", sizeof("main"))) {
                        vm.main = functions + i;
                        break;
                }
        }

        if (!vm.main) {
                fprintf(stderr, ascii(RED, "Can't find main function. Abort.\n"));
                free_array(&func_scope, sizeof(ac_symbol));
                destruct_stack(&symtab);
                return tree;
        }
        
        dump_symtab(&symtab);
$$
        array globals_scope = {};
        push_stack(&symtab, &globals_scope);

        compile_stmt(tree, &symtab, &vm);
$$
$       (dump_symtab(&symtab);)

        section_free(vm.secs + SEC_TEXT);
        section_free(vm.secs + SEC_BSS);
        section_free(vm.secs + SEC_RELA_TEXT);
$$
        free_array(&globals_scope, sizeof(ac_symbol));
$$
        if (vm.reg.stack) {
                fprintf(stderr, ascii(RED, "Unbalanced register stack (vm.reg.stack)\n"));
                free_array(&func_scope, sizeof(ac_symbol));
                free_array(&globals_scope, sizeof(ac_symbol));
                destruct_stack(&symtab);
                return tree;
        }
        
        compile_stmt(tree, &symtab, &vm);      
$$
        compile_start(&symtab, &vm);
        syms[SYM_START].value = vm._start;
$$
        free_array(&func_scope, sizeof(ac_symbol));
        free_array(&globals_scope, sizeof(ac_symbol));
        destruct_stack(&symtab);
$$
        
        return error;
}

inline static int is_global_scope(stack *symtabs) 
{
$$
        fprintf(logs, html(GREEN, "IS_GLOBAL: %d\n"), symtabs->size < 2);
        return symtabs->size < 3;
}

static void compile_start(stack *symtabs, ac_virtual_memory *vm) 
{
        assert(symtabs);
        assert(vm);

        array *global_symtab = (array *)top_stack(symtabs);
        ac_symbol *globals = (ac_symbol *)global_symtab->data;
        
        vm->_start = vm->secs[SEC_TEXT].size;

        /* call rel */
        struct __attribute__((packed)) {
                const ubyte opcode = 0xe8;
                imm32 imm          = 0;
        } call;
        
        /* Compile globals initialization */
        for (int i = 0; i < global_symtab->size; i++) {
                call.imm = globals[i].offset - vm->secs[SEC_TEXT].size - sizeof(call);
                encode(vm, &call, sizeof(call));                
        }

        /* call main */
        call.imm = vm->main->offset - vm->secs[SEC_TEXT].size - sizeof(call);
        encode(vm, &call, sizeof(call));  

        /* mov rax, 0x3c */
        struct __attribute__((packed)) {
                const ubyte opcode = 0xb8;
                const imm32 imm    = 0x3c;
        } mov2rax;
        encode(vm, &mov2rax, sizeof(mov2rax));

        /* xor rdi, rdi */
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x31;
                const ie64_modrm modrm  = { .rm = IE64_RDI, .reg = IE64_RDI, .mod = 0b11 };
        } xor_rdi;
        encode(vm, &xor_rdi, sizeof(xor_rdi));

        /* syscall */
        struct __attribute__((packed)) {
                const ubyte prefix = 0x0f;
                const ubyte opcode = 0x05;
        } syscall;
        encode(vm, &syscall, sizeof(syscall));
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

        if (keyword(root) == AST_CALL)
                return compile_call(root, symtabs, vm);
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
                        const ubyte rex    = 0x4d; /* 0b01001101 */
                        const ubyte opcode = 0x01;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
                } add;
$$
                add.modrm.reg = vm->reg.stack;
                vm->reg.stack--;
                add.modrm.rm = vm->reg.stack;

                encode(vm, &add, sizeof(add));
                return success(root); 
                       
        } else if (keyword(root) == AST_SUB) {
$$
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4d; /* 0b01001101 */
                        const ubyte opcode = 0x29;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
                } sub;
$$
                sub.modrm.reg = vm->reg.stack;
                vm->reg.stack--;
                sub.modrm.rm = vm->reg.stack;

                encode(vm, &sub, sizeof(sub));
                return success(root);     
                           
        } else if (keyword(root) == AST_MUL) {

                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4d; /* 0b01001101 */
                        const ubyte prefix = 0x0f;
                        const ubyte opcode = 0xaf;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
                } imul;
$$
                imul.modrm.rm = vm->reg.stack;
                vm->reg.stack--;
                imul.modrm.reg = vm->reg.stack;

                encode(vm, &imul, sizeof(imul));
                return success(root);     
                       
        } else if (keyword(root) == AST_DIV) {

                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4c; /* 0b01001100 */
                        const ubyte opcode = 0x89;
                        ie64_modrm modrm   = { .rm = IE64_RAX, .reg = 0b000, .mod = 0b11 };
                } mov2rax;

                mov2rax.modrm.reg = vm->reg.stack - 1;
                encode(vm, &mov2rax, sizeof(mov2rax));

                struct __attribute__((packed)) {
                        const ubyte rex    = 0x49; /* 0b1001001 */
                        const ubyte opcode = 0xf7;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = 0b111, .mod = 0b11 };
                } idiv;

                idiv.modrm.rm = vm->reg.stack;
                vm->reg.stack--;
                encode(vm, &idiv, sizeof(idiv));
                              
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x49; /* 0b1001001 */
                        const ubyte opcode = 0x89;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = IE64_RAX, .mod = 0b11 };
                } mov2reg;

                mov2reg.modrm.rm = vm->reg.stack;
                encode(vm, &mov2reg, sizeof(mov2reg));

                return success(root);
        }

        if (keyword(root) == AST_IN)
                return compile_stdcall(root, symtabs, vm, SYM_SCAN);

        if (keyword(root) == AST_NOT) {
        
                /* not reg */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x49; /* 1001001b */
                        const ubyte opcode = 0x83;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = 0b110, .mod = 0b11 };
                        const imm8 imm           = 1;
                } not_reg;

                not_reg.modrm.rm = vm->reg.stack;
                encode(vm, &not_reg, sizeof(not_reg));
                return success(root);
                
        } else if (keyword(root) == AST_AND) {

                /* and reg1, reg2 */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4d; /* 1001101b */
                        const ubyte opcode = 0x21;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
                } and_reg;

                and_reg.modrm.reg = vm->reg.stack;
                vm->reg.stack--;
                and_reg.modrm.rm = vm->reg.stack;
                
                encode(vm, &and_reg, sizeof(and_reg));
                return success(root);
                
        } else if (keyword(root) == AST_OR) {

                /* or reg1, reg2 */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4d; /* 1001101b */
                        const ubyte opcode = 0x09;
                        ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
                } and_reg;

                and_reg.modrm.reg = vm->reg.stack;
                vm->reg.stack--;
                and_reg.modrm.rm = vm->reg.stack;
                
                encode(vm, &and_reg, sizeof(and_reg));
                return success(root);
        }

        struct __attribute__((packed)) {
                const ubyte rex    = 0x4d; /* 1001101b */
                const ubyte opcode = 0x39;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
        } cmp;

        cmp.modrm.reg = vm->reg.stack;
        vm->reg.stack--;
        cmp.modrm.rm  = vm->reg.stack;

        encode(vm, &cmp, sizeof(cmp));

        struct __attribute__((packed)) {
                const ubyte rex        = 0x48; /* 1001000b */
                const ubyte opcode     = 0xc7;
                const ie64_modrm modrm = { .rm = IE64_RAX, .reg = 0b000, .mod = 0b11 }; 
                const imm32 imm        = 1;
        } mov_rax_true;
        
        encode(vm, &mov_rax_true, sizeof(mov_rax_true));


        struct __attribute__((packed)) {
                const ubyte rex    = 0x41; /* 1001001b */
                ubyte opcode       = 0xb8;
                imm32 imm          = 0;
        } zero_reg;

        zero_reg.opcode += vm->reg.stack;
        encode(vm, &zero_reg, sizeof(zero_reg));

        struct __attribute__((packed)) {
                const ubyte rex    = 0x4c; /* 1001100b */
                const ubyte prefix = 0x0f;
                ubyte opcode       = 0x00;
                ie64_modrm modrm   = { .rm = IE64_RAX, .reg = 0b000, .mod = 0b11 };
        } cmov;
        
        cmov.modrm.reg = vm->reg.stack;

        switch (keyword(root)) {
        case AST_EQUAL:
                cmov.opcode = 0x44;
                break;
        case AST_NEQUAL:
                cmov.opcode = 0x45;
                break;
        case AST_GREAT:
                cmov.opcode = 0x4F;
                break;;
        case AST_LOW:
                cmov.opcode = 0x4c;
                break;
        case AST_GEQUAL:
                cmov.opcode = 0x4d;
                break;
        case AST_LEQUAL:
                cmov.opcode = 0x4e;
                break;
        default:
$$              return syntax_error(root);
        }
$$
        encode(vm, &cmov, sizeof(cmov));
        return success(root);
}

static ast_node *compile_while(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);      

        ast_node *error = nullptr;

        require(root, AST_WHILE);

        if (!root->left)
                return syntax_error(root);

        ptrdiff_t while_offs = vm->secs[SEC_TEXT].size;
        
        error = compile_expr(root->left, symtabs, vm);
        if (error)
                return error;

        /* test reg, reg */
        struct __attribute__((packed)) {
                const ubyte rex     = 0x4d; /* 1001101b */
                const ubyte opcode  = 0x85;
                ie64_modrm modrm    = { .rm = 0b000, .reg = 0b000, .mod = 0b11 }; 
        } test;

        test.modrm.rm  = vm->reg.stack;
        test.modrm.reg = vm->reg.stack;
        vm->reg.stack--;

        encode(vm, &test, sizeof(test));

        /* je rel */
        struct __attribute__((packed)) {
                const ubyte prefix = 0x0f;
                const ubyte opcode = 0x84;
                imm32 imm          = 0;
        } je;

        je.imm = -vm->secs[SEC_TEXT].size - sizeof(je);
        ptrdiff_t je_end = encode(vm, &je, sizeof(je)) - vm->secs[SEC_TEXT].data;

        error = compile_stmt(root->right, symtabs, vm);
        if (error)
                return error;

        /* jmp rel */
        struct __attribute__((packed)) {
                const ubyte opcode = 0xe9;
                imm32 imm          = 0;
        } jmp;

        jmp.imm = while_offs - vm->secs[SEC_TEXT].size - sizeof(jmp);
        encode(vm, &jmp, sizeof(jmp));
        
        je.imm += vm->secs[SEC_TEXT].size;
        memcpy(je_end + vm->secs[SEC_TEXT].data, &je, sizeof(je));     

        return success(root);
}

static ast_node *compile_if(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);      

        ast_node *error = nullptr;
        require(root, AST_IF);

        error = compile_expr(root->left, symtabs, vm);
$$
        if (error)
                return error;

        /* test reg, reg */
        struct __attribute__((packed)) {
                const ubyte rex     = 0x4d; /* 1001101b */
                const ubyte opcode  = 0x85;
                ie64_modrm modrm    = { .rm = 0b000, .reg = 0b000, .mod = 0b11 }; 
        } test;

        test.modrm.rm  = vm->reg.stack;
        test.modrm.reg = vm->reg.stack;
        vm->reg.stack--;

        encode(vm, &test, sizeof(test));

        /* je rel */
        struct __attribute__((packed)) {
                const ubyte prefix = 0x0f;
                const ubyte opcode = 0x84;
                imm32 imm          = 0;
        } je;

        je.imm = -vm->secs[SEC_TEXT].size - sizeof(je);
        ptrdiff_t je_fail = encode(vm, &je, sizeof(je)) - vm->secs[SEC_TEXT].data;
        
        ast_node *decision = root->right;
        if (!decision)
                return syntax_error(root);
$$
        if (!decision->left)
                return syntax_error(root);     
$$
        error = compile_stmt(decision->left, symtabs, vm);
        if (error)
                return error;

        if (decision->right) {
                /* jmp rel */
                struct __attribute__((packed)) {
                        const ubyte opcode = 0xe9;
                        imm32 imm          = 0;
                } jmp;

                jmp.imm = -vm->secs[SEC_TEXT].size - sizeof(jmp);
                ptrdiff_t jmp_end = encode(vm, &jmp, sizeof(jmp)) - vm->secs[SEC_TEXT].data;

                je.imm += vm->secs[SEC_TEXT].size;
                memcpy(je_fail + vm->secs[SEC_TEXT].data, &je, sizeof(je));

                error = compile_stmt(decision->right, symtabs, vm);
                if (error)
                        return error;
                        
                jmp.imm += vm->secs[SEC_TEXT].size; 
                memcpy(jmp_end + vm->secs[SEC_TEXT].data, &jmp, sizeof(jmp));
                
        } else {

                je.imm += vm->secs[SEC_TEXT].size;
                memcpy(je_fail + vm->secs[SEC_TEXT].data, &je, sizeof(je));
        }

        return success(root);
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

                if (sym->addend >= 0){
                        mov.imm = -sym->addend - sym->info;
                } else {
                        mov.imm = -sym->addend;
                }

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
        
        if (sym->addend >= 0){
                mov.imm = -sym->addend - sym->info;
        } else {
                mov.imm = -sym->addend;
        }

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
                        const ubyte rex    = 0x4e; /* 1001110b */
                        ubyte opcode       = 0x00;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b00 };
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
                        const ubyte rex    = 0x4c; /* 1001100b */
                        ubyte opcode       = 0x00;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b00 };
                        const ie64_sib sib = { .base = 0b101, .index = 0b100, .scale = 0b00 };
                        imm32 imm          = 0;   
                } mov;          
$$
                rela.r_offset += 0x04;

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

        /* Encode ret for the start up initialization */
        if (is_global_scope(symtabs)) {
                /* ret */
                struct __attribute__((packed)) {
                        ubyte opcode = 0xc3;     
                } ret;
                
                encode(vm, &ret, sizeof(ret));   
        }      

        
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

static ast_node *compile_call(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs); 
        ast_node *error = nullptr;
$$
        require_ident(root->left);
        ac_symbol *sym = find_symbol(symtabs, ast_ident(root->left));
        if (!sym || sym->type != AC_SYM_FUNC)
                return syntax_error(root);

        size_t n_args = sym->info;

                     /* push r9 */
        struct __attribute__((packed)) {
                const ubyte rex = 0x41; /* 1000001b */
                ubyte opcode    = 0x50;
        } push_r9;

        push_r9.opcode += vm->reg.stack;
        encode(vm, &push_r9, sizeof(push_r9));    
       

        /* align stack */
        if (n_args % 2) {
                n_args++;
                /* sub rsp, 0x8 */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x48; /* 1001000b */
                        const ubyte opcode = 0x81;
                        const ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b101, .mod = 0b11 };
                        const imm32 imm          = 0x8;
                } align; 
                
                encode(vm, &align, sizeof(align));        
        }

        ast_node *param = root->right;
        /* push arguments */
        for (size_t i = 0; i < sym->info; i++) {
                if (!param)
                        return syntax_error(root);
                        
                error = compile_expr(param->right, symtabs, vm);
                if (error)
                        return error;

                param = param->left;
        
                /* push */
                struct __attribute__((packed)) {
                        const ubyte rex = 0x41; /* 1000001b */
                        ubyte opcode    = 0x50;
                } push;           
$$
                push.opcode += vm->reg.stack;
                vm->reg.stack--;
                
                encode(vm, &push, sizeof(push));        
        }
$$
        /* call */
        struct __attribute__((packed)) {
                const ubyte opcode = 0xe8;
                imm32 imm          = 0x00;
        } call; 
$$
        call.imm = sym->offset - vm->secs[SEC_TEXT].size - sizeof(call);     
        encode(vm, &call, sizeof(call));
$$
        /* mov reg, rax */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0b01001001;
                const ubyte opcode = 0x89;
                ie64_modrm modrm = { .rm = 0b000, .reg = IE64_RAX, .mod = 0b11 };         
        } mov_rax_top;      
$$        
        vm->reg.stack++;
        mov_rax_top.modrm.rm = vm->reg.stack;
        encode(vm, &mov_rax_top, sizeof(mov_rax_top));

        /* add rsp, text size */
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48; /* 1001000b */
                const ubyte opcode = 0x81;
                ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b000, .mod = 0b11 };
                imm32 imm          = 0;
        } add_rsp;

        add_rsp.imm = n_args * 0x8;
        encode(vm, &add_rsp, sizeof(add_rsp)); 
$$

        /* push r9 */
        struct __attribute__((packed)) {
                const ubyte rex = 0x41; /* 1000001b */
                ubyte opcode    = 0x58;
        } pop_r9;

        pop_r9.opcode += vm->reg.stack - 1;
        encode(vm, &pop_r9, sizeof(pop_r9));

        return success(root);        
}


static ast_node *compile_stdcall(ast_node *root, stack *symtabs, ac_virtual_memory *vm, const int sym_index)
{
        assert(vm);
        assert(root);
        assert(symtabs); 
        ast_node *error = nullptr;
$$
        elf64_symbol *sym = vm->syms + sym_index;
        assert(sym);
$$
        size_t n_args = sym->info;

        /* align stack */
        if (n_args % 2 == 0) {
                n_args++;
                /* sub rsp, 0x8 */
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x48; /* 1001000b */
                        const ubyte opcode = 0x81;
                        const ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b101, .mod = 0b11 };
                        const imm32 imm          = 0x8;
                } align; 
                
                encode(vm, &align, sizeof(align));        
        }

        /* push arguments */
        for (size_t i = 0; i < sym->info; i++) {
                /* push */
                struct __attribute__((packed)) {
                        const ubyte rex = 0x41; /* 1000001b */
                        ubyte opcode    = 0x50;
                } push;           
$$
                push.opcode += vm->reg.stack;
                vm->reg.stack--;
                
                encode(vm, &push, sizeof(push));        
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
        encode(vm, &mov_rax_top, sizeof(mov_rax_top));

        /* add rsp, text size */
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48; /* 1001000b */
                const ubyte opcode = 0x81;
                ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b000, .mod = 0b11 };
                imm32 imm          = 0;
        } add_rsp;

        add_rsp.imm = n_args * 0x8;
        encode(vm, &add_rsp, sizeof(add_rsp)); 
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
        /* mov rsp, rbp */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                ie64_modrm modrm = { .rm = IE64_RSP, .reg = IE64_RBP, .mod = 0b11 };         
        } mov_rsp_rbp;

        encode(vm, &mov_rsp_rbp, sizeof(mov_rsp_rbp));

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
        ast_node *stmt = root->right;
        switch (ast_keyword(stmt)) {
        case AST_ASSIGN:
$$
                return compile_assign(stmt, symtabs, vm);

        case AST_DEFINE:
                return compile_define(stmt, symtabs, vm);

        case AST_IF:
$$
                return compile_if(root->right, symtabs, vm);
        case AST_WHILE:
$$
                return compile_while(root->right, symtabs, vm);
        case AST_SHOW:
                fprintf(stderr, "AST_SHOW is not supported\n");
                return syntax_error(root);
        case AST_CALL:
$$
                error = compile_call(root->right, symtabs, vm);
                if (error)
                        return error;
                vm->reg.stack--;
                
                return success(root);
        case AST_OUT:
$$              error = compile_expr(stmt->right, symtabs, vm);
                if (error)
                        return syntax_error(root);
                
                error = compile_stdcall(stmt, symtabs, vm, SYM_PRINT);
                if (error)
                        return syntax_error(root);
                /* Don't need return value */
                vm->reg.stack--;
                return success(root);
                
        case AST_RETURN:
$$
                return compile_return(stmt, symtabs, vm);
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
        dump_symtab(symtabs);
        array symtab = {};
        push_stack(symtabs, &symtab);
$$
        dump_symtab(symtabs);

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
                        .addend = -8 * (n_params + 1),
                        .offset = 0,
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
        /* mov rbp, rsp */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                ie64_modrm modrm = { .rm = IE64_RBP, .reg = IE64_RSP, .mod = 0b11 };         
        } mov_rsp_rbp;
$$
        encode(vm, &mov_rsp_rbp, sizeof(mov_rsp_rbp));
$$
        /* sub rsp, text size */
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48; /* 1001000b */
                const ubyte opcode = 0x81;
                ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b101, .mod = 0b11 };
                imm32 imm          = 0;
        } sub_rsp; 

        ptrdiff_t sub_rsp_addr = encode(vm, &sub_rsp, sizeof(sub_rsp)) - vm->secs[SEC_TEXT].data;
$$
        error = compile_stmt(root->right, symtabs, vm);
        if (error) {
                free_array(&symtab, sizeof(ac_symbol));
                return error;
        }
$$
        /* Patch sub rbp, size */
        sub_rsp.imm = elf64_align(vm->secs[SEC_NULL].size, 0x10);
        memcpy(sub_rsp_addr + vm->secs[SEC_TEXT].data, &sub_rsp, sizeof(sub_rsp));

        pop_stack(symtabs);
        free_array(&symtab, sizeof(ac_symbol));
        section_free(vm->secs + SEC_NULL);
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

static char *encode(ac_virtual_memory *vm, void *instruction, size_t size)
{
        assert(vm);
        assert(instruction);
        return section_memcpy(vm->secs + SEC_TEXT, instruction, size);
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
