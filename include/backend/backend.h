#ifndef BACKEND_H
#define BACKEND_H

#include <elf.h>
#include <stack.h>
#include <ast/tree.h>
#include <backend/iencode.h>

enum ac_symbol_type {
        SYM_UND     = 0,
        SYM_VAR     = 1,
        SYM_ARRAY   = 2,
        SYM_CONST   = 3,
        SYM_FUNC    = 4,
};

enum ac_symbol_vis {
        VIS_UND    = 0,
        VIS_LOCAL  = STB_LOCAL,
        VIS_GLOBAL = STB_GLOBAL,
};

struct ac_symbol {
        int type = SYM_UND;
        int vis  = VIS_UND;

        const char *ident = nullptr;
        ast_node *node    = nullptr;
        ptrdiff_t offset  = 0;

        union {
                size_t size = 0;
                size_t n_args;
        } info; 
};

struct ac_segment {
        size_t elf64_ndx_shdr   = 0;
        size_t elf64_ndx_symtab = 0;
        
        size_t size       = 0;
        char *data        = 0;
        size_t allocated  = 0;
};

const size_t SEG_ALLOC_INIT = 256;

void  segment_free (ac_segment *segment);
void *segment_alloc(ac_segment *segment, size_t size);

size_t segment_memcpy(
        ac_segment *segment, 
        const char *data, 
        size_t size
);

char *segment_mmap(
        ac_segment *segment, 
        const char *data, 
        size_t size
);


struct ac_virt_machine {
        struct {
                int stack = 0;
                const int ret  = IE_RAX;
                const int call = IE_RDI; 
        } reg;

        union {
                /* For ease of iteration */
                ac_segment *segments[6];
                struct {
                        ac_segment *rodata = nullptr;
                        ac_segment *text   = nullptr;
                        ac_segment *data   = nullptr;
                        ac_segment *bss    = nullptr;
                        ac_segment *stack  = nullptr;
                        ac_segment *undef  = nullptr;
                } segment;                   
        };
}; 

struct ac_symtab {
        ptrdiff_t _start = 0;
        
        stack symbols   = {};
        stack rela_text = {};        
};


#endif /* BACKEND_H */
