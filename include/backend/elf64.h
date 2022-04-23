#ifndef ASSERT_ELF64_H
#define ASSERT_ELF64_H

#include <elf.h>
#include <iommap.h>
#include <backend/backend.h>


const size_t ASS_ELF_ALIGN = 0x10;


/*   0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS file
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .text
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    2 .rodata
     4: 0000000000000000     0 SECTION LOCAL  DEFAULT    3 .data
     5: 0000000000000000     0 SECTION LOCAL  DEFAULT    4 .bss
     6: stdlib functions
     ............................................................
     n: xxxxxxxxxxxxxxxx     0 NOTYPE  GLOBAL DEFAULT    1 _start */

const Elf64_Half ASS_ELF_LOCALSNUM = 6;
const Elf64_Half ASS_ELF_STDLIBNUM = 1;
const Elf64_Half ASS_ELF_SYMTABNUM = ASS_ELF_LOCALSNUM + ASS_ELF_STDLIBNUM + 1;


const Elf64_Half ASS_ELF_SHNUM  = 9;

enum elf64_shdr_ndx {
        ASS_ELF_NDX_NULL        = 0,      
        ASS_ELF_NDX_TEXT        = 1,      
        ASS_ELF_NDX_RODATA      = 2,      
        ASS_ELF_NDX_DATA        = 3,      
        ASS_ELF_NDX_BSS         = 4,      
        ASS_ELF_NDX_SHSTRTAB    = 5,      
        ASS_ELF_NDX_SYMTAB      = 6,      
        ASS_ELF_NDX_STRTAB      = 7,      
        ASS_ELF_NDX_RELA_TEXT   = 8,      
};

enum elf64_symtab_ndx {
        ASS_SYM_NDX_NULL        = 0,      
        ASS_SYM_NDX_FILE        = 1,      
        ASS_SYM_NDX_TEXT        = 2,      
        ASS_SYM_NDX_RODATA      = 3,      
        ASS_SYM_NDX_DATA        = 4,      
        ASS_SYM_NDX_BSS         = 5,            
};


int create_elf64(ac_virt_machine *vm, ac_symtab *st, const char *name);


#endif /* ASSERT_ELF64_H */
