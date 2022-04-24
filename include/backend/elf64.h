#ifndef ASSERT_ELF64_H
#define ASSERT_ELF64_H

#include <elf.h>
#include <iommap.h>
#include <backend/backend.h>


typedef ac_segment elf64_section;

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


int create_elf64(
        elf64_section *secs, elf64_symbol *syms, const char *name
);


#endif /* ASSERT_ELF64_H */
