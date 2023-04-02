#ifndef ASSERT_ELF64_H
#define ASSERT_ELF64_H

#include <elf.h>
#include <iommap.h>

struct elf64_section {
        const char *name  = 0;
        size_t size       = 0;
        char *data        = 0;
        size_t allocated  = 0;
};

const size_t SEC_ALIGN = 0x10;
enum elf64_sections_enum {
        SEC_NULL      = 0,      
        SEC_TEXT      = 1,      
        SEC_RODATA    = 2,      
        SEC_DATA      = 3,      
        SEC_BSS       = 4,      
        SEC_SHSTRTAB  = 5,      
        SEC_SYMTAB    = 6,      
        SEC_STRTAB    = 7,      
        SEC_RELA_TEXT = 8,
        SEC_NUM  
};

struct elf64_symbol {   
        char *name = nullptr;
        ptrdiff_t value = 0;
        size_t info = 0;
};

/* Import standard functions */
enum elf64_symbols_enum {
        SYM_NULL   = 0,       
        SYM_FILE   = 1,       
        SYM_TEXT   = 2,       
        SYM_RODATA = 3,       
        SYM_DATA   = 4,       
        SYM_BSS    = 5,   
            
#define ASS_STDLIB(ID, NAME, ARGS)     \
        SYM_##ID,
#include "../STDLIB"
#undef ASS_STDLIB

        SYM_START, 
        SYM_NUM
};

const size_t SYM_LOCALS = SYM_BSS + 1;

inline size_t elf64_align(size_t addr, size_t align = SEC_ALIGN);
int create_elf64(elf64_section *secs, elf64_symbol *syms, const char *name);

elf64_symbol  *fill_symbols_info  (elf64_section *secs, elf64_symbol *syms, const char *file_name);
elf64_section *fill_sections_names(elf64_section *secs);

int elf64_utest(const char *file_name);

#endif /* ASSERT_ELF64_H */
