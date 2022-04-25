#include <elf.h>
#include <logs.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iommap.h>
#include <backend/elf64.h>
#include <backend/backend.h>

static Elf64_Ehdr *elf64_create_ehdr();

extern const size_t SEC_ALIGN;
static inline size_t elf64_align(size_t addr, size_t align = SEC_ALIGN);


int elf64_utest(const char *file_name)
{  
        assert(file_name);
            
        elf64_section *secs = (elf64_section *)calloc(SEC_NUM, sizeof(elf64_section));
        assert(secs);

        elf64_symbol *syms = (elf64_symbol *)calloc(SYM_NUM, sizeof(elf64_symbol));
        assert(syms);

        fill_sections_names(secs);
        fill_symbols_names(secs, syms, file_name);

        unsigned char text_data[] = {
                0xc3, 0xe8, 0xfa, 0xff, 0xff, 0xff, 0x48, 0x31, 
                0xc0, 0xc3, 0xbf, 0x0d, 0x00, 0x00, 0x00, 0x48,
                0x31, 0xc0, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x48,
                0x89, 0x14, 0x25, 0x00, 0x00, 0x00, 0x00, 0xbf,
                0x0c, 0x00, 0x00, 0x00, 0xe8, 0x00, 0x00, 0x00,
                0x00, 0x48, 0x8b, 0x0c, 0x25, 0x00, 0x00, 0x00,
                0x00, 0x48, 0x8b, 0x0c, 0x25, 0x00, 0x00, 0x00,
                0x00, 0x48, 0x8b, 0x0c, 0x25, 0x00, 0x00, 0x00,
                0x00, 0x48, 0x8b, 0x0c, 0x25, 0x00, 0x00, 0x00,
                0x00, 0xe8, 0xb2, 0xff, 0xff, 0xff, 0xe8, 0xae,
                0xff, 0xff, 0xff, 0x48, 0x89, 0x14, 0x25, 0x00,
                0x00, 0x00, 0x00, 0xb8, 0x3c, 0x00, 0x00, 0x00,
                0x48, 0x31, 0xff, 0x0f, 0x05, 0xc3
        };

        section_mmap(secs + SEC_TEXT, text_data, sizeof(text_data));

        unsigned char rodata_data[] = {
                0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0xbe, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        section_mmap(secs + SEC_RODATA, rodata_data, sizeof(rodata_data));

        unsigned char data_data[] = {
                0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x23, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        section_mmap(secs + SEC_DATA, data_data, sizeof(data_data));

        secs[SEC_BSS].size = 0x30;
        
        syms[SYM_START].value = 0xa;

        elf64_section *rela = secs + SEC_RELA_TEXT;
        /* Fill text relocation table */
        Elf64_Rela rela0 = {0x13, ELF64_R_INFO(SYM_PRINT,  R_X86_64_PC32), -0x04}; section_memcpy(rela, &rela0, sizeof(Elf64_Rela));
        Elf64_Rela rela1 = {0x1b, ELF64_R_INFO(SYM_BSS,    R_X86_64_32S),   0x00}; section_memcpy(rela, &rela1, sizeof(Elf64_Rela));
        Elf64_Rela rela2 = {0x25, ELF64_R_INFO(SYM_PRINT,  R_X86_64_PC32), -0x04}; section_memcpy(rela, &rela2, sizeof(Elf64_Rela));
        Elf64_Rela rela3 = {0x2d, ELF64_R_INFO(SYM_RODATA, R_X86_64_32S),   0x00}; section_memcpy(rela, &rela3, sizeof(Elf64_Rela));
        Elf64_Rela rela4 = {0x35, ELF64_R_INFO(SYM_RODATA, R_X86_64_32S),   0x08}; section_memcpy(rela, &rela4, sizeof(Elf64_Rela));
        Elf64_Rela rela5 = {0x3d, ELF64_R_INFO(SYM_DATA,   R_X86_64_32S),   0x00}; section_memcpy(rela, &rela5, sizeof(Elf64_Rela));
        Elf64_Rela rela6 = {0x45, ELF64_R_INFO(SYM_DATA,   R_X86_64_32S),   0x08}; section_memcpy(rela, &rela6, sizeof(Elf64_Rela));
        Elf64_Rela rela7 = {0x57, ELF64_R_INFO(SYM_BSS,    R_X86_64_32S),   0x10}; section_memcpy(rela, &rela7, sizeof(Elf64_Rela));

        create_elf64(secs, syms, file_name);
        
        for (size_t i = 0; i < SEC_NUM; i++)
                if (secs[i].data)
                        section_free(secs + i);
        
        free(syms);
        free(secs);

        return 0;
}

/* Section Headers:
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] .text             PROGBITS        0000000000000000 000280 000066 00  AX  0   0 16
  [ 2] .rodata           PROGBITS        0000000000000000 0002f0 000010 00   A  0   0  4
  [ 3] .data             PROGBITS        0000000000000000 000300 000010 00  WA  0   0  4
  [ 4] .bss              NOBITS          0000000000000000 000310 000030 00  WA  0   0  4
  [ 5] .shstrtab         STRTAB          0000000000000000 000310 00003f 00      0   0  1
  [ 6] .symtab           SYMTAB          0000000000000000 000350 000180 18      7  14  8
  [ 7] .strtab           STRTAB          0000000000000000 0004d0 000044 00      0   0  1
  [ 8] .rela.text        RELA            0000000000000000 000520 0000c0 18      6   1  8 */

static Elf64_Shdr *elf64_create_shdrs(elf64_section *secs) 
{
        assert(secs);


        Elf64_Shdr *shdrs = (Elf64_Shdr *)calloc(SEC_NUM, sizeof(Elf64_Shdr));
        assert(shdrs);
        if (!shdrs)
                return nullptr;
        
        size_t size      = 0;
        Elf64_Off offset = elf64_align(sizeof(Elf64_Ehdr)) +
                           elf64_align(sizeof(Elf64_Shdr) * SEC_NUM);

        char *shstrtab = secs[SEC_SHSTRTAB].data;

        offset += elf64_align(size);
        size = secs[SEC_TEXT].size;
        shdrs[SEC_TEXT] = {
                .sh_name      = secs[SEC_TEXT].name - shstrtab,
                .sh_type      = SHT_PROGBITS,
                .sh_flags     = SHF_ALLOC | SHF_EXECINSTR,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = 0,
                .sh_info      = 0,
                .sh_addralign = 0x10,
                .sh_entsize   = 0,
        };

        offset += elf64_align(size);
        size = secs[SEC_RODATA].size;
        shdrs[SEC_RODATA] = {
                .sh_name      = secs[SEC_RODATA].name - shstrtab,
                .sh_type      = SHT_PROGBITS,
                .sh_flags     = SHF_ALLOC,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = 0,
                .sh_info      = 0,
                .sh_addralign = 0x4,
                .sh_entsize   = 0,
        };

        offset += elf64_align(size);
        size = secs[SEC_DATA].size;
        shdrs[SEC_DATA] = {
                .sh_name      = secs[SEC_DATA].name - shstrtab,
                .sh_type      = SHT_PROGBITS,
                .sh_flags     = SHF_ALLOC | SHF_WRITE,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = 0,
                .sh_info      = 0,
                .sh_addralign = 0x4,
                .sh_entsize   = 0,
        };        

        offset += elf64_align(size);
        size = secs[SEC_BSS].size;
        shdrs[SEC_BSS] = {
                .sh_name      = secs[SEC_BSS].name - shstrtab,
                .sh_type      = SHT_NOBITS,
                .sh_flags     = SHF_ALLOC | SHF_WRITE,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = 0,
                .sh_info      = 0,
                .sh_addralign = 0x4,
                .sh_entsize   = 0,
        };

        size = secs[SEC_SHSTRTAB].size;
        shdrs[SEC_SHSTRTAB] = {
                .sh_name      = secs[SEC_SHSTRTAB].name - shstrtab,
                .sh_type      = SHT_STRTAB,
                .sh_flags     = 0,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = 0,
                .sh_info      = 0,
                .sh_addralign = 0x1,
                .sh_entsize   = 0,
        };

        offset += elf64_align(size);
        size = SYM_NUM * sizeof(Elf64_Sym);
        shdrs[SEC_SYMTAB] = {
                .sh_name      = secs[SEC_SYMTAB].name - shstrtab,
                .sh_type      = SHT_SYMTAB,
                .sh_flags     = 0,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = SEC_STRTAB,
                .sh_info      = SYM_LOCALS,
                .sh_addralign = 0x8,
                .sh_entsize   = sizeof(Elf64_Sym),
        };

        offset += elf64_align(size);
        size = secs[SEC_STRTAB].size;
        shdrs[SEC_STRTAB] = {
                .sh_name      = secs[SEC_STRTAB].name - shstrtab,
                .sh_type      = SHT_STRTAB,
                .sh_flags     = 0,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = 0,
                .sh_info      = 0,
                .sh_addralign = 0x1,
                .sh_entsize   = 0,
        };

        offset += elf64_align(size);
        size = secs[SEC_RELA_TEXT].size;
        shdrs[SEC_RELA_TEXT] = {
                .sh_name      = secs[SEC_RELA_TEXT].name - shstrtab,
                .sh_type      = SHT_RELA,
                .sh_flags     = 0,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = SEC_SYMTAB,
                .sh_info      = SEC_TEXT,
                .sh_addralign = 0x8,
                .sh_entsize   = sizeof(Elf64_Rela),
        };

        return shdrs;
}


/*   0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS file
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .text
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    2 .rodata
     4: 0000000000000000     0 SECTION LOCAL  DEFAULT    3 .data
     5: 0000000000000000     0 SECTION LOCAL  DEFAULT    4 .bss
     6: stdlib functions
     ............................................................
     n: xxxxxxxxxxxxxxxx     0 NOTYPE  GLOBAL DEFAULT    1 _start */
     
Elf64_Sym *elf64_create_symtab(elf64_symbol *syms, elf64_section *secs)
{
        Elf64_Sym *symtab = (Elf64_Sym *)calloc(SYM_NUM, sizeof(Elf64_Sym));
        assert(symtab);
        if (!symtab)
                return nullptr;

        char *strtab = secs[SEC_STRTAB].data;
     
        symtab[SYM_FILE] = {
                .st_name  = syms[SYM_FILE].name - strtab,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_FILE),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = SHN_ABS,    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[SYM_TEXT] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = SEC_TEXT,    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[SYM_RODATA] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = SEC_RODATA,    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[SYM_DATA] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = SEC_DATA,    
                .st_value = 0,
                .st_size  = 0,  
        };
        
        symtab[SYM_BSS] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = SEC_BSS,    
                .st_value = 0,
                .st_size  = 0,  
        };


#define ASS_STDLIB(ID, NAME, ARGS)                                      \
        symtab[SYM_##ID] = {                                            \
                .st_name  = syms[SYM_##ID].name - strtab,               \
                .st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE),      \
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),           \
                .st_shndx = 0,                                          \
                .st_value = 0,                                          \
                .st_size  = 0,                                          \
        };
        
        #include "../STDLIB"
#undef ASS_STDLIB 

        symtab[SYM_START] = {
                .st_name  = syms[SYM_START].name - strtab,
                .st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = SEC_TEXT,    
                .st_value = syms[SYM_START].value,
                .st_size  = 0,  
        };
        
        return symtab;
}

elf64_symbol *fill_symbols_names(elf64_section *secs, elf64_symbol *syms, const char *file_name)
{
        assert(syms);

        elf64_section *strtab = secs + SEC_STRTAB;

        syms[SYM_NULL].name = section_memcpy(strtab, "", sizeof(""));        
        syms[SYM_FILE].name = section_memcpy(strtab, file_name, strlen(file_name) + 1);

#define ASS_STDLIB(ID, NAME, ARGS) \
        syms[SYM_##ID].name = section_memcpy(strtab, NAME, sizeof(NAME));
#include "../STDLIB"
#undef ASS_STDLIB

        syms[SYM_START].name = section_memcpy(strtab, "_start", sizeof("_start"));

        return nullptr;
}


elf64_section *fill_sections_names(elf64_section *secs)
{
        assert(secs);
        
        elf64_section *shstrtab = secs + SEC_SHSTRTAB;
        
        secs[SEC_NULL].name      = section_memcpy(shstrtab, "", sizeof(""));
        secs[SEC_TEXT].name      = section_memcpy(shstrtab, ".text", sizeof(".text"));
        secs[SEC_RODATA].name    = section_memcpy(shstrtab, ".rodata", sizeof(".rodata"));
        secs[SEC_DATA].name      = section_memcpy(shstrtab, ".data", sizeof(".data"));
        secs[SEC_BSS].name       = section_memcpy(shstrtab, ".bss", sizeof(".bss"));
        secs[SEC_SHSTRTAB].name  = section_memcpy(shstrtab, ".shstrtab", sizeof(".shstrtab"));
        secs[SEC_SYMTAB].name    = section_memcpy(shstrtab, ".symtab", sizeof(".symtab"));
        secs[SEC_STRTAB].name    = section_memcpy(shstrtab, ".strtab", sizeof(".strtab"));
        secs[SEC_RELA_TEXT].name = section_memcpy(shstrtab, ".rela.text", sizeof(".rela.text"));   

        return shstrtab;
}

int create_elf64(elf64_section *secs, elf64_symbol *syms, const char *name)
{

        Elf64_Ehdr *ehdr = elf64_create_ehdr();
        assert(ehdr);

        Elf64_Sym *symtab = elf64_create_symtab(syms, secs);
        assert(symtab);

        Elf64_Shdr *shdrs = elf64_create_shdrs(secs);
        assert(shdrs);

        size_t elf64_size = elf64_align(
                shdrs[SEC_NUM - 1].sh_size   + 
                shdrs[SEC_NUM - 1].sh_offset
        );

        fprintf(stderr, "elf64_size: %lu\n", elf64_size);
        
        mmap_data md = {
                .buf  = nullptr,
                .size = elf64_size,
        };        
        
        mmap_out(&md, name);
        
        memcpy(md.buf, ehdr,  sizeof(Elf64_Shdr));
        memcpy(md.buf + ehdr->e_shoff, shdrs, sizeof(Elf64_Shdr) * SEC_NUM);
        memcpy(md.buf + shdrs[SEC_SYMTAB].sh_offset, symtab, shdrs[SEC_SYMTAB].sh_size);

        for (size_t i = 0; i < SEC_NUM; i++)
                if (secs[i].data && i != SEC_SYMTAB)
                        memcpy(md.buf + shdrs[i].sh_offset, secs[i].data, shdrs[i].sh_size);

        mmap_free(&md);

        free(ehdr);
        free(shdrs);
        free(symtab);
        
        return errno;
}

static Elf64_Ehdr *elf64_create_ehdr()
{
        unsigned char e_ident[EI_NIDENT] = {0};

        e_ident[EI_MAG0] = ELFMAG0;
        e_ident[EI_MAG1] = ELFMAG1;
        e_ident[EI_MAG2] = ELFMAG2;
        e_ident[EI_MAG3] = ELFMAG3;

        e_ident[EI_CLASS]   = ELFCLASS64;
        e_ident[EI_DATA]    = ELFDATA2LSB;
        e_ident[EI_VERSION] = EV_CURRENT;
        e_ident[EI_OSABI]   = ELFOSABI_NONE;        

        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)calloc(1, sizeof(Elf64_Ehdr));
        assert(ehdr);
        
        *ehdr = {
                .e_ident        = {0},
                .e_type         = ET_REL,
                .e_machine      = EM_X86_64,
                .e_version      = EV_CURRENT,
                .e_entry        = 0,
                .e_phoff        = 0,
                .e_shoff        = sizeof(Elf64_Ehdr),
                .e_flags        = 0,
                .e_ehsize       = sizeof(Elf64_Ehdr),
                .e_phentsize    = 0,
                .e_phnum        = 0,
                .e_shentsize    = sizeof(Elf64_Shdr),
                .e_shnum        = SEC_NUM,
                .e_shstrndx     = SEC_SHSTRTAB,            
        };

        memcpy(ehdr->e_ident, e_ident, EI_NIDENT);
        return ehdr;
}

static inline size_t elf64_align(size_t addr, size_t align)
{
        return addr + (align - addr) % align; 
}
        
char *section_memcpy(
        elf64_section *sec, 
        const void *data, 
        size_t size
) {
        assert(data);
        assert(sec);
        
        section_alloc(sec, size);
        memcpy(sec->data + sec->size, data, size);

        char *ret = sec->data + sec->size;
        sec->size += size; 

        return ret;
}

char *section_mmap(
        elf64_section *sec, 
        const void *data, 
        size_t size
) {
        assert(data);
        assert(sec);
        assert(!sec->data);
        
        if (!section_alloc(sec, size))
                return nullptr;
                
        memcpy(sec->data, data, size);
        sec->size = size;
         
        return sec->data;
}

void section_free(elf64_section *sec)
{
        assert(sec);
        sec->allocated = 0;
        
        free(sec->data);
        sec->data = nullptr;
}

void *section_alloc(elf64_section *sec, size_t size) 
{ 
        assert(sec);
        assert(size);

        char *data       = sec->data;
        size_t allocated = sec->allocated;

        if (!allocated)
                allocated = SEG_ALLOC_INIT;   

        while (size + sec->size > allocated)
                allocated *= 2;

        if (allocated > sec->allocated) {
                data = (char *)realloc(sec->data, allocated);
                if (!data) {
                        int saved_errno = errno;

                        fprintf(stderr, "Can't allocate segment data: %s", strerror(errno));

                        errno = saved_errno;
                        return nullptr;
                }        
        }

        sec->allocated = allocated;
        sec->data      = data;
        
        return data;
}



