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

static ast_node *compile_stdcall(ast_node *root, stack *symtabs, ac_virtual_memory *vm, const int sym_index);
static ast_node *compile_define (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_return (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_assign (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_expr   (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_while  (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_call   (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_stmt   (ast_node *root, stack *symtabs, ac_virtual_memory *vm);
static ast_node *compile_if     (ast_node *root, stack *symtabs, ac_virtual_memory *vm);

static void compile_start(stack *symtabs, ac_virtual_memory *vm);

static ast_node *compile_load_num(ast_node *root, stack *symtabs, ac_virtual_memory *vm);

static ast_node *compile_stack_store(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym);
static ast_node *compile_stack_load (ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym);

static ast_node *compile_data_store(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym);
static ast_node *compile_data_load (ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym);

static void dump_symtab(stack *symtabs); 

static ast_node *declare_functions(ast_node *root, stack *symtabs);

static char *emit(ac_virtual_memory *vm, void *instruction, size_t size);


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

        dump_symtab(&symtab);
$$
        array globals_scope = {};
        push_stack(&symtab, &globals_scope);

        if (!vm.main) {
                fprintf(stderr, ascii(RED, "Can't find main function. Abort.\n"));
                goto cleanup;
        }
        
        error = compile_stmt(tree, &symtab, &vm);
        if (error) {
                fprintf(stderr, ascii(RED, "Compilation error.\n"));
                goto cleanup;
        }
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
                goto cleanup;
        }
        
        compile_stmt(tree, &symtab, &vm);      
$$
        compile_start(&symtab, &vm);
        syms[SYM_START].value = vm._start;
$$
cleanup:

        free_array(&func_scope, sizeof(ac_symbol));
        free_array(&globals_scope, sizeof(ac_symbol));
        destruct_stack(&symtab);
$$
        return error;
}

static inline int is_global_scope(stack *symtabs) 
{
$$
        fprintf(logs, html(GREEN, "IS_GLOBAL: %d\n"), symtabs->size < 2);
        return symtabs->size < 3;
}

static inline void patch(ac_virtual_memory *vm, ptrdiff_t addr, void *instruction, size_t size)
{
        assert(vm);
        assert(instruction);
        memcpy(vm->secs[SEC_TEXT].data + addr, instruction, size);
}

/* Returns the current rip register value.
   i.e. returns the offset of the start of 
   the next command on the given. */
static inline ptrdiff_t rip(ac_virtual_memory *vm) 
{
        assert(vm); 
        return vm->secs[SEC_TEXT].size; 
}

static void emit_syscall(ac_virtual_memory *vm, const ubyte rax);

static inline char *emit(ac_virtual_memory *vm, void *instruction, size_t size)
{
        assert(vm);
        assert(instruction);
        return section_memcpy(vm->secs + SEC_TEXT, instruction, size);
}

static void compile_start(stack *symtabs, ac_virtual_memory *vm) 
{
        assert(symtabs);
        assert(vm);

        array *global_symtab = (array *)top_stack(symtabs);
        ac_symbol *globals = (ac_symbol *)global_symtab->data;

        /* Compile _start code at the 
           end of the .text section. */
        vm->_start = rip(vm);

        /* call near */
        struct __attribute__((packed)) {
                const ubyte opcode = 0xe8; /* call */
                imm32 imm          = 0;    /* displacement relative to next instruction */ 
        } __call;
        
        /* Compile startup initialization of global variables. */
        for (int i = 0; i < global_symtab->size; i++) {
                __call.imm = globals[i].offset - rip(vm) - sizeof(__call);
                emit(vm, &__call, sizeof(__call));                
        }

        /* Call the main() function. */
        __call.imm = vm->main->offset - rip(vm) - sizeof(__call);
        emit(vm, &__call, sizeof(__call));

        /* Move rax to rdi */        
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                ie64_modrm modrm = { .rm = IE64_RDI, .reg = IE64_RAX, .mod = 0b11 };         
        } __mov32;
        emit(vm, &__mov32, sizeof(__mov32));

        /* Emit exit() syscall. */
        emit_syscall(vm, 0x3c);
}

static void emit_syscall(ac_virtual_memory *vm, const ubyte rax)
{
        assert(vm);

        /* Move imm32 to rax */
        struct __attribute__((packed)) {
                const ubyte opcode = 0xb8 + IE64_RAX; /* mov rax, imm */
                imm32 imm          = 0;
        } __mov;
        
        __mov.imm = rax;
        emit(vm, &__mov, sizeof(__mov));

        struct __attribute__((packed)) {
                const ubyte prefix = 0x0f;
                const ubyte opcode = 0x05;
        } __syscall;
        
        emit(vm, &__syscall, sizeof(__syscall));
}

static ast_node *compile_store(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym)
{
        assert(vm);
        assert(sym);
        assert(root);
        assert(symtabs); 
        ast_node *error = nullptr;

        if (sym->vis == AC_VIS_LOCAL) {
                return compile_stack_store(root, symtabs, vm, sym);      
        } else if (sym->vis == AC_VIS_GLOBAL) {
                
                error = compile_data_store(root, symtabs, vm, sym);
                if (error)
                        return error;

                /* Compile ret for the startup initialization */
                if (is_global_scope(symtabs)) {
                        struct __attribute__((packed)) {
                                ubyte opcode = 0xc3;     
                        } __ret;                

                        emit(vm, &__ret, sizeof(__ret));
                }

                return success(root);
        }        

        return syntax_error(root);
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
        require_ident(root->left);
        ac_symbol sym = {
                .type   = AC_SYM_VAR,
                .vis    = AC_VIS_LOCAL,
                .ident  = ast_ident(root->left),
                .node   = root,
                .addend = 0,
                .offset = rip(vm),
                .info   = 8,               
        };

        error = compile_expr(root->right, symtabs, vm);
        if (error)
                return error;
$$
        ac_symbol *exist = find_symbol(symtabs, ast_ident(root->left));
        if (exist) {
                /* Can't reassign variables at global scope */
                if (is_global_scope(symtabs))
                        return syntax_error(root);

                return compile_store(root->left, symtabs, vm, exist);
        }
$$
        ast_node *size = root->left->right;
        if (size) {
                require_number(size);
                sym.info = 8 * (ast_number(size) + 1);
        }
$$
        elf64_section *sec = nullptr;
        if (is_global_scope(symtabs)) {
                sym.vis    = AC_VIS_GLOBAL;
                sec = vm->secs + SEC_BSS;
                sym.addend = sec->size;
        } else {
                sym.vis    = AC_VIS_LOCAL;
                sec = vm->secs + SEC_NULL;
                sym.addend = - sec->size - sym.info; 
        }
        
        sec->size += sym.info;
$$
        array_push((array *)top_stack(symtabs), &sym, sizeof(ac_symbol));
$       (dump_symtab(symtabs);)
$$
        return compile_store(root->left, symtabs, vm, &sym);
}

static ast_node *compile_expr_add(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{                       
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4d; /* 0b01001101 */
                const ubyte opcode = 0x01;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
        } __add;
$$
        __add.modrm.reg = vm->reg.stack--;
        __add.modrm.rm  = vm->reg.stack;

        emit(vm, &__add, sizeof(__add));
        
        return success(root);        
}

static ast_node *compile_expr_sub(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4d; /* 0b01001101 */
                const ubyte opcode = 0x29;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
        } __sub;
$$
        __sub.modrm.reg = vm->reg.stack--;
        __sub.modrm.rm  = vm->reg.stack;

        emit(vm, &__sub, sizeof(__sub));
        
        return success(root);        
}

static ast_node *compile_expr_mul(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4d; /* 0b01001101 */
                const ubyte prefix = 0x0f;
                const ubyte opcode = 0xaf;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
        } __imul;
$$
        __imul.modrm.rm  = vm->reg.stack--;
        __imul.modrm.reg = vm->reg.stack;

        emit(vm, &__imul, sizeof(__imul));
        
        return success(root); 
}

static ast_node *compile_expr_div(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4c; /* 0b01001100 */
                const ubyte opcode = 0x89;
                ie64_modrm modrm   = { .rm = IE64_RAX, .reg = 0b000, .mod = 0b11 };
        } __mov32;

        __mov32.modrm.reg = (vm->reg.stack - 1);
        emit(vm, &__mov32, sizeof(__mov32));

        /* Sign extend rax to rdx:rax */
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x99;
        } __cqo;
        emit(vm, &__cqo, sizeof(__cqo));

        struct __attribute__((packed)) {
                const ubyte rex    = 0x49; /* 0b1001001 */
                const ubyte opcode = 0xf7;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b111, .mod = 0b11 };
        } __idiv;

        __idiv.modrm.rm = vm->reg.stack--;
        emit(vm, &__idiv, sizeof(__idiv));
                      
        struct __attribute__((packed)) {
                const ubyte rex    = 0x49; /* 0b1001001 */
                const ubyte opcode = 0x89;
                ie64_modrm modrm   = { .rm = 0b000, .reg = IE64_RAX, .mod = 0b11 };
        } __mov64;

        __mov64.modrm.rm = vm->reg.stack;
        emit(vm, &__mov64, sizeof(__mov64));

        return success(root);        
}

static ast_node *compile_expr_not(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0x49; /* 1001001b */
                const ubyte opcode = 0x83;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b110, .mod = 0b11 };
                const imm8 imm     = 1;
        } __not;

        __not.modrm.rm = vm->reg.stack;
        emit(vm, &__not, sizeof(__not));
        
        return success(root);        
}
static ast_node *compile_expr_and(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4d; /* 1001101b */
                const ubyte opcode = 0x21;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
        } __and;

        __and.modrm.reg = vm->reg.stack--;
        __and.modrm.rm  = vm->reg.stack;
        
        emit(vm, &__and, sizeof(__and));
        
        return success(root);
}
static ast_node *compile_expr_or(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4d; /* 1001101b */
                const ubyte opcode = 0x09;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
        } __or;

        __or.modrm.reg = vm->reg.stack--;
        __or.modrm.rm  = vm->reg.stack;
        
        emit(vm, &__or, sizeof(__or));
        return success(root);
}

static ast_node *compile_expr_cmp(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0x4d; /* 1001101b */
                const ubyte opcode = 0x39;
                ie64_modrm modrm   = { .rm = 0b000, .reg = 0b000, .mod = 0b11 };
        } __cmp;

        __cmp.modrm.reg = vm->reg.stack--;
        __cmp.modrm.rm  = vm->reg.stack;

        emit(vm, &__cmp, sizeof(__cmp));

        /* mov rax, 0x01 */
        struct __attribute__((packed)) {
                ubyte opcode       = 0xb8 + IE64_RAX;
                imm32 imm          = 1;
        } __mov32;

        emit(vm, &__mov32, sizeof(__mov32));                

        /* Zero register. */
        struct __attribute__((packed)) {
                const ubyte rex    = 0x41; /* 1001001b */
                ubyte opcode       = 0xb8;
                imm32 imm          = 0;
        } __mov64;

        __mov64.opcode += vm->reg.stack;
        emit(vm, &__mov64, sizeof(__mov64));

        struct __attribute__((packed)) {
                const ubyte rex    = 0x4c; /* 1001100b */
                const ubyte prefix = 0x0f;
                ubyte opcode       = 0x00;
                ie64_modrm modrm   = { .rm = IE64_RAX, .reg = 0b000, .mod = 0b11 };
        } __cmov;
        
        __cmov.modrm.reg = vm->reg.stack;

        switch (keyword(root)) {
        case AST_EQUAL:
                __cmov.opcode = 0x44;
                break;
        case AST_NEQUAL:
                __cmov.opcode = 0x45;
                break;
        case AST_GREAT:
                __cmov.opcode = 0x4F;
                break;;
        case AST_LOW:
                __cmov.opcode = 0x4c;
                break;
        case AST_GEQUAL:
                __cmov.opcode = 0x4d;
                break;
        case AST_LEQUAL:
                __cmov.opcode = 0x4e;
                break;
        default:
$$              return syntax_error(root);
        }
$$
        emit(vm, &__cmov, sizeof(__cmov));
        
        return success(root);        
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

        if (root->type == AST_NODE_NUMBER)
                return compile_load_num(root, symtabs, vm);
        
        if (root->type == AST_NODE_IDENT) {
                ac_symbol *sym = find_symbol(symtabs, ast_ident(root));
                if (!sym)
                        return syntax_error(root);

                if (sym->vis == AC_VIS_LOCAL)
                        return compile_stack_load(root, symtabs, vm, sym);
                else if (sym->vis == AC_VIS_GLOBAL)
                        return compile_data_load (root, symtabs, vm, sym);

                return syntax_error(root);
        }

        switch (keyword(root)) {
        case AST_IN:
                return compile_stdcall (root, symtabs, vm, SYM_SCAN);
        case AST_ADD:
                return compile_expr_add(root, symtabs, vm);
        case AST_SUB:
                return compile_expr_sub(root, symtabs, vm);
        case AST_MUL:
                return compile_expr_mul(root, symtabs, vm);
        case AST_DIV:
                return compile_expr_div(root, symtabs, vm);
        case AST_NOT:
                return compile_expr_not(root, symtabs, vm);
        case AST_AND:
                return compile_expr_and(root, symtabs, vm);
        case AST_OR:
                return compile_expr_or (root, symtabs, vm);
        case AST_EQUAL:
        case AST_NEQUAL:
        case AST_GREAT:
        case AST_LOW:
        case AST_GEQUAL:
        case AST_LEQUAL:
                return compile_expr_cmp(root, symtabs, vm);
        default:
                return syntax_error(root);
        }
}


/* Сompiles the following pattern:
            ...        
      ┌───> (condition)
      │     test       
      │ ┌── je         
      │ │   (body)
      └─┼───jmp     
        └─> ...        
*/    
static ast_node *compile_while(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);      

        ast_node *error = nullptr;
        require(root, AST_WHILE);

        if (!root->left)
                return syntax_error(root);

        /* Store the start of the condition code */
        ptrdiff_t cond_addr = rip(vm);

        /* Compile condition expression */        
        error = compile_expr(root->left, symtabs, vm);
        if (error)
                return error;

        struct __attribute__((packed)) {
                const ubyte rex     = 0x4d; /* 1001101b */
                const ubyte opcode  = 0x85; /* test */
                ie64_modrm modrm    = { .rm = 0b000, .reg = 0b000, .mod = 0b11 }; 
        } __test;

        __test.modrm.rm  = vm->reg.stack;
        __test.modrm.reg = vm->reg.stack--;

        emit(vm, &__test, sizeof(__test));

        struct __attribute__((packed)) {
                const ubyte prefix = 0x0f;
                const ubyte opcode = 0x84;
                imm32 imm          = 0;
        } __je;

        ptrdiff_t __je_addr = rip(vm);
        
        /* Jump to the end of cycle */
        emit(vm, &__je, sizeof(__je));

        /* Compile body loop */
        error = compile_stmt(root->right, symtabs, vm);
        if (error)
                return error;

        struct __attribute__((packed)) {
                const ubyte opcode = 0xe9;
                imm32 imm          = 0;
        } __jmp;

        __jmp.imm = cond_addr - rip(vm) - sizeof(__jmp);

        /* Jump to the condition expression */
        emit(vm, &__jmp, sizeof(__jmp));

        /* Patch jump to the end of loop */
        __je.imm = -__je_addr - sizeof(__je) + rip(vm);
        patch(vm, __je_addr, &__je, sizeof(__je));

        return success(root);
}

/* Сompiles the following patterns:

        ...              OR        ...        
	(condition)                (condition)
        test                       test       
     ┌─ je                      ┌─ je         
     │  (true body)             │  (true body)     
     └>  ...                  ┌─┼──jmp       
                              │ └> (false body)
                              └──> ...     
*/  
static ast_node *compile_if(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);      

        ast_node *error = nullptr;
        require(root, AST_IF);

        error = compile_expr(root->left, symtabs, vm);
        if (error)
                return error;

        struct __attribute__((packed)) {
                const ubyte rex     = 0x4d; /* 1001101b */
                const ubyte opcode  = 0x85;
                ie64_modrm modrm    = { .rm = 0b000, .reg = 0b000, .mod = 0b11 }; 
        } __test;

        __test.modrm.rm  = vm->reg.stack;
        __test.modrm.reg = vm->reg.stack--;

        emit(vm, &__test, sizeof(__test));

        struct __attribute__((packed)) {
                const ubyte prefix = 0x0f;
                const ubyte opcode = 0x84;
                imm32 imm          = 0;
        } __je;

        ptrdiff_t __je_addr = rip(vm);
        emit(vm, &__je, sizeof(__je));
        
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

                struct __attribute__((packed)) {
                        const ubyte opcode = 0xe9;
                        imm32 imm          = 0;
                } __jmp;

                ptrdiff_t __jmp_addr = rip(vm);
                emit(vm, &__jmp, sizeof(__jmp));
                
                __je.imm = rip(vm) - __je_addr - sizeof(__je);
                patch(vm, __je_addr, &__je, sizeof(__je));

                error = compile_stmt(decision->right, symtabs, vm);
                if (error)
                        return error;
                        
                __jmp.imm = rip(vm) - __jmp_addr - sizeof(__jmp);
                patch(vm, __jmp_addr, &__jmp, sizeof(__jmp));
                
        } else {
                __je.imm = rip(vm) - __je_addr - sizeof(__je);
                patch(vm, __je_addr, &__je, sizeof(__je));
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
        struct __attribute__((packed)) {
                const ubyte rex = 0b01001001;
                ubyte opcode    = 0xb8;  
                imm64 imm       = 0;   
        } __movabs;
$$
        __movabs.imm = (imm64)ast_number(root);
        __movabs.opcode += (++vm->reg.stack);
        emit(vm, &__movabs, sizeof(__movabs));
$$
        return success(root);
}

static ast_node *compile_stack_store(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym)
{
        assert(vm);
        assert(sym);
        assert(root);
        
        ast_node *error = nullptr;
        require_ident(root);        

        if (root->right) {

                ast_node *error = compile_expr(root->right, symtabs, vm);
                if (error)
                        return error;

                struct __attribute__((packed)) {
                        const ubyte rex    = 0b01001110;
                        const ubyte opcode = 0x89;
                        ie64_modrm modrm   = { .rm   = 0b100, .reg = 0b000, .mod = 0b10 };
                        ie64_sib sib       = { .base = IE64_RBP, .index = 0b000, .scale = 0b11 };  
                        imm32 imm          = 0;   
                } __mov64;
      
                __mov64.sib.index = vm->reg.stack--;
                __mov64.modrm.reg = vm->reg.stack--;               
                __mov64.imm       = sym->addend;

                /* mov [rbp + 8*r + imm], r */
                emit(vm, &__mov64, sizeof(__mov64));
                return success(root);
                
        } else {

                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4c; /* 1001100b */
                        const ubyte opcode = 0x89;
                        ie64_modrm modrm   = { .rm = IE64_RBP, .reg = 0b000, .mod = 0b10 };
                        imm32 imm          = 0;   
                } __mov64;

                __mov64.modrm.reg = vm->reg.stack--;
                __mov64.imm       = sym->addend;

                /* mov [rbp + imm], r */
                emit(vm, &__mov64, sizeof(__mov64));
                return success(root);
        }
}      

static ast_node *compile_stack_load(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym)
{
        assert(vm);
        assert(sym);
        assert(root);
        
        ast_node *error = nullptr;
        require_ident(root);        

        if (root->right) {

                ast_node *error = compile_expr(root->right, symtabs, vm);
                if (error)
                        return error;
                        
                struct __attribute__((packed)) {
                        const ubyte rex    = 0b01001110;
                        const ubyte opcode = 0x8b;
                        ie64_modrm modrm   = { .rm   = 0b100, .reg = 0b000, .mod = 0b10 };
                        ie64_sib sib       = { .base = IE64_RBP, .index = 0b000, .scale = 0b11 };  
                        imm32 imm          = 0;   
                } __mov64;
      
                __mov64.sib.index = vm->reg.stack;
                __mov64.modrm.reg = vm->reg.stack;               
                __mov64.imm       = sym->addend;

                /* mov r, [rbp + 8*r + imm] */
                emit(vm, &__mov64, sizeof(__mov64));
                return success(root);
                
        } else {
        
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4c; /* 1001100b */
                        const ubyte opcode = 0x8b;
                        ie64_modrm modrm   = { .rm = IE64_RBP, .reg = 0b000, .mod = 0b10 };
                        imm32 imm          = 0;   
                } __mov64;

                __mov64.modrm.reg = (++vm->reg.stack);
                __mov64.imm       = sym->addend;

                /* mov r, [rbp + imm] */
                emit(vm, &__mov64, sizeof(__mov64));
                return success(root);
        }
}      

static ast_node *compile_data_store(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym)
{
        assert(vm);
        assert(sym);
        assert(root);
        
        ast_node *error = nullptr;
        require_ident(root); 

        Elf64_Rela rela = {
                .r_offset = 0, 
                .r_info   = ELF64_R_INFO(SYM_BSS, R_X86_64_32S), 
                .r_addend = sym->addend,
        };  

        if (root->right) {

               error = compile_expr(root->right, symtabs, vm);
                if (error)
                        return error;
                                
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4e; /* 1001110b */
                        const ubyte opcode = 0x89;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b00 };
                        ie64_sib sib       = { .base = 0b101, .index = 0b000, .scale = 0b11 };
                        imm32 imm          = 0;   
                } __mov64;          
$$
                rela.r_offset = rip(vm) + 0x04;
$$                
                __mov64.sib.index = vm->reg.stack--;
                __mov64.modrm.reg = vm->reg.stack--;
$$
                /* mov [8*r + imm], r */
                emit(vm, &__mov64, sizeof(__mov64));       
                
        } else {
        
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4c; /* 1001100b */
                        const ubyte opcode = 0x89;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b00 };
                        const ie64_sib sib = { .base = 0b101, .index = 0b100, .scale = 0b00 };
                        imm32 imm          = 0;   
                } __mov64;          
$$
                rela.r_offset = rip(vm) + 0x04;

                __mov64.modrm.reg = vm->reg.stack--;
$$     
                /* mov [imm], r */
                emit(vm, &__mov64, sizeof(__mov64));
        }
$$
        section_memcpy(vm->secs + SEC_RELA_TEXT, &rela, sizeof(Elf64_Rela));
        return success(root);
}

static ast_node *compile_data_load(ast_node *root, stack *symtabs, ac_virtual_memory *vm, ac_symbol *sym)
{
        assert(vm);
        assert(sym);
        assert(root);
        
        ast_node *error = nullptr;
        require_ident(root); 

        Elf64_Rela rela = {
                .r_offset = 0, 
                .r_info   = ELF64_R_INFO(SYM_BSS, R_X86_64_32S), 
                .r_addend = sym->addend,
        };  

        if (root->right) {

                ast_node *error = compile_expr(root->right, symtabs, vm);
                if (error)
                        return error;
                                
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4e; /* 1001110b */
                        const ubyte opcode = 0x8b;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b00 };
                        ie64_sib sib       = { .base = 0b101, .index = 0b000, .scale = 0b11 };
                        imm32 imm          = 0;   
                } __mov64;          
$$
                rela.r_offset = rip(vm) + 0x04;
$$                
                __mov64.sib.index = vm->reg.stack;
                __mov64.modrm.reg = vm->reg.stack;
$$
                /* mov r, [8*r + imm] */
                emit(vm, &__mov64, sizeof(__mov64));       
                
        } else {
        
                struct __attribute__((packed)) {
                        const ubyte rex    = 0x4c; /* 1001100b */
                        const ubyte opcode = 0x8b;
                        ie64_modrm modrm   = { .rm = 0b100, .reg = 0b000, .mod = 0b00 };
                        const ie64_sib sib = { .base = 0b101, .index = 0b100, .scale = 0b00 };
                        imm32 imm          = 0;   
                } __mov64;          
$$
                rela.r_offset = rip(vm) + 0x04;

                __mov64.modrm.reg = (++vm->reg.stack);
$$     
                /* mov r, [imm] */
                emit(vm, &__mov64, sizeof(__mov64));
        }
$$
        section_memcpy(vm->secs + SEC_RELA_TEXT, &rela, sizeof(Elf64_Rela));
        return success(root);
}

static ast_node *compile_call_begin(ast_node *root, ac_virtual_memory *vm, int *pushed)
{
        /* Stack must be aligned to 0x10 boundary.
           i.e. (n_pushed + return address) % 2 */
        size_t n_pushed = *pushed;
        if ((n_pushed + 1) % 2) {

                n_pushed++;
                struct __attribute__((packed)) {
                        const ubyte rex          = 0x48; /* 1001000b */
                        const ubyte opcode       = 0x81;
                        const ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b101, .mod = 0b11 }; /* 0b11 0b101 - добавитть описание */
                        const imm32 imm          = 0x8;
                } __sub; 
                
                /* sub rsp, 0x8 */
                emit(vm, &__sub, sizeof(__sub));        
        }

        *pushed = n_pushed;
        return success(root);
}

static ast_node *compile_call_end(ast_node *root, ac_virtual_memory *vm, int pushed)
{
        struct __attribute__((packed)) {
                const ubyte rex    = 0b01001001;
                const ubyte opcode = 0x89;
                ie64_modrm modrm = { .rm = 0b000, .reg = IE64_RAX, .mod = 0b11 };         
        } __mov64;      
$$        
        __mov64.modrm.rm = (++vm->reg.stack);

        /* mov r, rax */
        emit(vm, &__mov64, sizeof(__mov64));

        struct __attribute__((packed)) {
                const ubyte rex    = 0x48; /* 1001000b */
                const ubyte opcode = 0x81;
                ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b000, .mod = 0b11 };
                imm32 imm          = 0;
        } __add;

        __add.imm = pushed * 0x8;

        /* add rsp, aligned * 0x8 */
        emit(vm, &__add, sizeof(__add)); 

        return success(root);
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

        /* We have to save registers because 
           called function can change their values. */
        int n_saved = vm->reg.stack;
        for (int i = 0; i < n_saved; i++) {
                struct __attribute__((packed)) {
                        const ubyte rex = 0x41; /* 1000001b */
                        ubyte opcode    = 0x50;
                } __push;

                __push.opcode += (i + 1);
                emit(vm, &__push, sizeof(__push));         
        }

        int n_pushed = sym->info;
        error = compile_call_begin(root, vm, &n_pushed);
        if (error)
                return error;
                
        /* Calculate and push function parameters */
        ast_node *param = root->right;
        for (size_t i = 0; i < sym->info; i++) {
                if (!param)
                        return syntax_error(root);
                        
                error = compile_expr(param->right, symtabs, vm);
                if (error)
                        return error;

                struct __attribute__((packed)) {
                        const ubyte rex = 0x41; /* 1000001b */
                        ubyte opcode    = 0x50;
                } __push;           
$$
                __push.opcode += vm->reg.stack--;
                emit(vm, &__push, sizeof(__push));        

                param = param->left;
        }

        struct __attribute__((packed)) {
                const ubyte opcode = 0xe8;
                imm32 imm          = 0x00;
        } __call; 
$$
        __call.imm = sym->offset - rip(vm) - sizeof(__call);     
        emit(vm, &__call, sizeof(__call));
        
        error = compile_call_end(root, vm, n_pushed);
        if (error)
                return error;

        /* Load the values of the saved registers */
        for (int i = n_saved; i > 0; i--) {

                struct __attribute__((packed)) {
                        const ubyte rex = 0x41; /* 1000001b */
                        ubyte opcode    = 0x58;
                } __pop;

                __pop.opcode += i;
                emit(vm, &__pop, sizeof(__pop));        
        }
                
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
        if (!sym)
                return syntax_error(root);

        int n_pushed = sym->info;
        error = compile_call_begin(root, vm, &n_pushed);
        if (error)
                return error;
$$
        /* Push arguments */
        for (size_t i = 0; i < sym->info; i++) {

                struct __attribute__((packed)) {
                        const ubyte rex = 0x41; /* 1000001b */
                        ubyte opcode    = 0x50;
                } __push;           
$$
                __push.opcode += (vm->reg.stack--);
                emit(vm, &__push, sizeof(__push));        
        }
$$
        struct __attribute__((packed)) {
                const ubyte opcode = 0xe8;
                imm32 imm          = 0x00;
        } __call; 
$$
        Elf64_Rela rela = {
                .r_offset = rip(vm) + 0x01, 
                .r_info   = ELF64_R_INFO(sym_index, R_X86_64_PC32), 
                .r_addend = -0x04,
        }; 
$$     
        section_memcpy(vm->secs + SEC_RELA_TEXT, &rela, sizeof(Elf64_Rela));      
        emit(vm, &__call, sizeof(__call));

        error = compile_call_end(root, vm, n_pushed);
        if (error)
                return error;
                
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
        
        } __mov64;
$$
        __mov64.modrm.reg = vm->reg.stack--;
        emit(vm, &__mov64, sizeof(__mov64));
$$
        /* mov rsp, rbp */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                ie64_modrm modrm = { .rm = IE64_RSP, .reg = IE64_RBP, .mod = 0b11 };         
        } __mov32;

        emit(vm, &__mov32, sizeof(__mov32));

        /* pop rbp */
        struct __attribute__((packed)) {
                const ubyte opcode = 0x58 + IE64_RBP;
        } __pop;
$$
        emit(vm, &__pop, sizeof(__pop));
$$
        struct __attribute__((packed)) {
                ubyte opcode = 0xc3;     
        } __ret;
      
        emit(vm, &__ret, sizeof(__ret));
$$
        return success(root);
}

static ast_node *compile_stmt(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);

        ast_node *error = nullptr;
        require(root, AST_STMT);

        if (root->left) {
                error = compile_stmt(root->left, symtabs, vm);
                if (error)
                        return error;
        }

        if (!root->right || !keyword(root))
                return syntax_error(root);
$$
        ast_node *stmt = root->right;
        switch (ast_keyword(stmt)) {
        case AST_ASSIGN:
                return compile_assign(stmt, symtabs, vm);
        case AST_DEFINE:
                return compile_define(stmt, symtabs, vm);
        case AST_IF:
                return compile_if(root->right, symtabs, vm);
        case AST_WHILE:
                return compile_while(root->right, symtabs, vm);
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
                /* Don(t need return value */
                vm->reg.stack--;
                return success(root);
                
        case AST_RETURN:
                return compile_return(stmt, symtabs, vm);
        default:
                return syntax_error(root);
        }
}

static ast_node *compile_define(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);
$$
        ast_node *error = nullptr;
        require(root, AST_DEFINE);
$$
        ast_node *function = root->left;
        require(function, AST_FUNC);
$$
        ast_node *name = function->left;
        require_ident(name);   
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
                        .addend = 8 * (n_params + 1),
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
        /* push rbp */
        struct __attribute__((packed)) {
                const ubyte opcode = 0x50 + IE64_RBP;
        } __push;
$$
        emit(vm, &__push, sizeof(__push));
$$
        /* mov rbp, rsp */         
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48;
                const ubyte opcode = 0x89;
                ie64_modrm modrm   = { .rm = IE64_RBP, .reg = IE64_RSP, .mod = 0b11 };         
        } __mov64;
$$
        emit(vm, &__mov64, sizeof(__mov64));
$$
        struct __attribute__((packed)) {
                const ubyte rex    = 0x48; /* 1001000b */
                const ubyte opcode = 0x81;
                ie64_modrm modrm   = { .rm = IE64_RSP, .reg = 0b101, .mod = 0b11 };
                imm32 imm          = 0;
        } __sub; 

        ptrdiff_t __sub_addr = rip(vm);
        emit(vm, &__sub, sizeof(__sub));
$$
        error = compile_stmt(root->right, symtabs, vm);
        if (error) {
                free_array(&symtab, sizeof(ac_symbol));
                return error;
        }
$$
        /* sub rbp, stack frame size */
        __sub.imm = elf64_align(vm->secs[SEC_NULL].size, 0x10);
        patch(vm, __sub_addr, &__sub, sizeof(__sub));

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
