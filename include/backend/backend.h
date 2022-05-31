#ifndef BACKEND_H
#define BACKEND_H

#include <elf.h>
#include <stack.h>
#include <ast/tree.h>
#include <backend/iencode.h>
#include <backend/elf64.h>

enum ac_symbol_type {
        AC_SYM_UND     = 0,
        AC_SYM_VAR     = 1,
        AC_SYM_ARRAY   = 2,
        AC_SYM_CONST   = 3,
        AC_SYM_FUNC    = 4,
};

enum ac_symbol_vis {
        AC_VIS_UND    = 0,
        AC_VIS_LOCAL  = 1,
        AC_VIS_GLOBAL = 2,
};

struct ac_symbol {
        int type = AC_SYM_UND;
        int vis  = AC_VIS_UND;

        const char *ident = nullptr;
        ast_node *node    = nullptr;
        imm32 addend      = 0;
        ptrdiff_t offset  = 0;
        ptrdiff_t info    = 0;
};

const size_t SEG_ALLOC_INIT = 256;

void  section_free (elf64_section *sec);
void *section_alloc(elf64_section *sec, size_t size);

char *section_memcpy(
        elf64_section *sec, 
        const void *data, 
        size_t size
);

char *section_mmap(
        elf64_section *sec, 
        const void *data, 
        size_t size
);

struct ac_virtual_memory {
        ptrdiff_t _start = 0;
        ac_symbol *main  = 0;
        struct {
                int stack = 0;
                const int ret  = IE64_RAX;
                const int call = IE64_RDI; 
        } reg;

        elf64_section *secs = nullptr;
        elf64_symbol  *syms = nullptr;
}; 

ast_node *compile_tree(ast_node *tree, elf64_section *secs, elf64_symbol *syms);

#endif /* BACKEND_H */
