#include <elf.h>
#include <logs.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iommap.h>
#include <backend/elf64.h>
#include <backend/backend.h>

static Elf64_Ehdr *elf64_create_ehdr();
int create_elf64(elf64_sections *sects, elf64_symbols *syms, const char *name);

static elf64_symbol *fill_symbols_names(elf64_sections *sects, elf64_symbols *syms, const char *file_name);
static elf64_section *fill_sections_names(elf64_sections *sects);

extern const size_t ASS_ELF_ALIGN;
static inline size_t elf64_align(size_t addr, size_t align = ASS_ELF_ALIGN);

elf64_section *sections_ctor(elf64_sections *sects)
{
        assert(sects);

        const size_t n_sections = sizeof(elf64_sections) / sizeof(elf64_section *);
        elf64_section *sections = (elf64_section *)calloc(n_sections, sizeof(elf64_section));
        assert(sections);

        elf64_section **sects_arr = (elf64_section **)sects;
        for (size_t i = 0; i < n_sections; i++) {
                sects_arr[i] = &sections[i];
                fprintf(stderr, "n_sections: %lu\n", n_sections);
                fprintf(stderr, "segments: %p\n", sects_arr[i]);
        }

        return sections;
}

elf64_symbol *symbols_ctor(elf64_symbols *syms)
{
        assert(syms);
        
        const size_t n_symbols = sizeof(elf64_symbols) / sizeof(elf64_symbol *);
        elf64_symbol *symbols = (elf64_symbol *)calloc(n_symbols, sizeof(elf64_symbol));
        assert(symbols);

        elf64_symbol **syms_arr = (elf64_symbol **)syms;
        for (size_t i = 0; i < n_symbols; i++)
                syms_arr[i] = &symbols[i];

        return symbols;
}

int main()
{
        elf64_sections sects = {};
        elf64_section *sections = sections_ctor(&sects);

        elf64_symbols syms = {};
        elf64_symbol *symbols = symbols_ctor(&syms);

        fill_sections_names(&sects);
        fill_symbols_names(&sects, &syms, "unit");

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

        segment_mmap(sects.text, text_data, sizeof(text_data));

        unsigned char rodata_data[] = {
                0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0xbe, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        segment_mmap(sects.rodata, rodata_data, sizeof(rodata_data));

        unsigned char data_data[] = {
                0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x23, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        segment_mmap(sects.data, data_data, sizeof(data_data));

        sects.bss->size = 0x30;
        
        syms._start->value = 0xa;

        elf64_section *rela = sects.rela_text;
        /* Fill text relocation table */
        Elf64_Rela rela0 = {0x13, ELF64_R_INFO(0x6, R_X86_64_PC32), -0x04}; segment_memcpy(rela, &rela0, sizeof(Elf64_Rela));
        Elf64_Rela rela1 = {0x1b, ELF64_R_INFO(0x5, R_X86_64_32S),   0x00}; segment_memcpy(rela, &rela1, sizeof(Elf64_Rela));
        Elf64_Rela rela2 = {0x25, ELF64_R_INFO(0x6, R_X86_64_PC32), -0x04}; segment_memcpy(rela, &rela2, sizeof(Elf64_Rela));
        Elf64_Rela rela3 = {0x2d, ELF64_R_INFO(0x3, R_X86_64_32S),   0x00}; segment_memcpy(rela, &rela3, sizeof(Elf64_Rela));
        Elf64_Rela rela4 = {0x35, ELF64_R_INFO(0x3, R_X86_64_32S),   0x08}; segment_memcpy(rela, &rela4, sizeof(Elf64_Rela));
        Elf64_Rela rela5 = {0x3d, ELF64_R_INFO(0x4, R_X86_64_32S),   0x00}; segment_memcpy(rela, &rela5, sizeof(Elf64_Rela));
        Elf64_Rela rela6 = {0x45, ELF64_R_INFO(0x4, R_X86_64_32S),   0x08}; segment_memcpy(rela, &rela6, sizeof(Elf64_Rela));
        Elf64_Rela rela7 = {0x57, ELF64_R_INFO(0x5, R_X86_64_32S),   0x10}; segment_memcpy(rela, &rela7, sizeof(Elf64_Rela));

        create_elf64(&sects, &syms, "unit");
        
        free(symbols);
        free(sections);

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

static inline size_t section_ndx(elf64_sections *sects, elf64_section *sect)
{
        return (sect - sects->null);
}

static inline size_t symbol_ndx(elf64_symbols *syms, elf64_symbol *sym)
{
        return (sym - syms->null);
}

static Elf64_Shdr *elf64_create_shdrs(elf64_sections *sects) 
{
        assert(sects);

        const size_t n_sections = sizeof(elf64_sections) / sizeof(elf64_section *);
        Elf64_Shdr *shdrs = (Elf64_Shdr *)calloc(n_sections, sizeof(Elf64_Shdr));
        assert(shdrs);
        if (!shdrs)
                return nullptr;
        
        size_t size      = 0;
        Elf64_Off offset = elf64_align(sizeof(Elf64_Ehdr)) + 
                           elf64_align(sizeof(Elf64_Shdr) * n_sections);

        char *shstrtab = sects->shstrtab->data;

        offset += elf64_align(size);
        size = sects->text->size;
        shdrs[section_ndx(sects, sects->text)] = {
                .sh_name      = sects->text->name - shstrtab,
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
        size = sects->rodata->size;
        shdrs[section_ndx(sects, sects->rodata)] = {
                .sh_name      = sects->rodata->name - shstrtab,
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
        size = sects->data->size;
        shdrs[section_ndx(sects, sects->data)] = {
                .sh_name      = sects->data->name - shstrtab,
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
        size = sects->bss->size;
        shdrs[section_ndx(sects, sects->bss)] = {
                .sh_name      = sects->bss->name - shstrtab,
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

        size = sects->shstrtab->size;
        shdrs[section_ndx(sects, sects->shstrtab)] = {
                .sh_name      = sects->shstrtab->name - shstrtab,
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
        size = sizeof(elf64_symbols) / sizeof(elf64_symbol *) * sizeof(Elf64_Sym);
        shdrs[section_ndx(sects, sects->symtab)] = {
                .sh_name      = sects->symtab->name - shstrtab,
                .sh_type      = SHT_SYMTAB,
                .sh_flags     = 0,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = ASS_ELF_NDX_STRTAB,
                .sh_info      = ASS_ELF_LOCALSNUM,
                .sh_addralign = 0x8,
                .sh_entsize   = sizeof(Elf64_Sym),
        };

        offset += elf64_align(size);
        size = sects->strtab->size;
        shdrs[section_ndx(sects, sects->strtab)] = {
                .sh_name      = sects->strtab->name - shstrtab,
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
        size = sects->rela_text->size;
        shdrs[section_ndx(sects, sects->rela_text)] = {
                .sh_name      = sects->rela_text->name - shstrtab,
                .sh_type      = SHT_RELA,
                .sh_flags     = 0,
                .sh_addr      = 0,
                .sh_offset    = offset,
                .sh_size      = size,
                .sh_link      = ASS_ELF_NDX_SYMTAB,
                .sh_info      = ASS_ELF_NDX_TEXT,
                .sh_addralign = 0x8,
                .sh_entsize   = sizeof(Elf64_Rela),
        };

        fprintf(stderr, "ndx: %p %p %lu\n", sects->null, sects->rela_text, section_ndx(sects, sects->rela_text));

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
     
Elf64_Sym *elf64_create_symtab(elf64_symbols *syms, elf64_sections *sects)
{
        Elf64_Sym *symtab = (Elf64_Sym *)calloc(sizeof(elf64_symbols) / sizeof(elf64_symbol *), sizeof(Elf64_Sym));
        assert(symtab);
        if (!symtab)
                return nullptr;

        elf64_section *strtab = sects->strtab;
     
        symtab[symbol_ndx(syms, syms->file)] =  {
                .st_name        = syms->file->name - strtab->data,
                .st_info        = ELF64_ST_INFO(STB_LOCAL, STT_FILE),
                .st_other       = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx       = SHN_ABS,    
                .st_value       = 0,
                .st_size        = 0,  
        };

        symtab[symbol_ndx(syms, syms->text)] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = section_ndx(sects, sects->text),    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[symbol_ndx(syms, syms->rodata)] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = section_ndx(sects, sects->rodata),    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[symbol_ndx(syms, syms->data)] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = section_ndx(sects, sects->data),    
                .st_value = 0,
                .st_size  = 0,  
        };
        
        symtab[symbol_ndx(syms, syms->bss)] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = section_ndx(sects, sects->bss),    
                .st_value = 0,
                .st_size  = 0,  
        };
        
        symtab[symbol_ndx(syms, syms->print)] = {
                .st_name  = syms->print->name - sects->strtab->data,
                .st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = 0,    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[symbol_ndx(syms, syms->_start)] = {
                .st_name  = syms->_start->name - sects->strtab->data,
                .st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = section_ndx(sects, sects->text),    
                .st_value = syms->_start->value,
                .st_size  = 0,  
        };

        free(sects->symtab->data);
        sects->symtab->data      = (char *)symtab;
        sects->symtab->allocated = sizeof(elf64_symbols) / sizeof(elf64_symbol *) * sizeof(Elf64_Sym);
        sects->symtab->size      = sizeof(elf64_symbols) / sizeof(elf64_symbol *) * sizeof(Elf64_Sym);
        
        return symtab;
}

static elf64_symbol *fill_symbols_names(elf64_sections *sects, elf64_symbols *syms, const char *file_name)
{
        assert(syms);

        elf64_section *strtab = sects->strtab;

        syms->null->name   = segment_memcpy(strtab, "", sizeof(""));        
        syms->file->name   = segment_memcpy(strtab, file_name, strlen(file_name) + 1);

#define ASS_STDLIB(NAME, ARGS) \
        syms->NAME->name   = segment_memcpy(strtab, #NAME, sizeof(#NAME));
#include "../STDLIB"
#undef ASS_STDLIB

        syms->_start->name = segment_memcpy(strtab, "_start", sizeof("_start"));

        return nullptr;
}


static elf64_section *fill_sections_names(elf64_sections *sects)
{
        assert(sects);
        
        elf64_section *shstrtab = sects->shstrtab;
        
        sects->null->name         = segment_memcpy(shstrtab, "", sizeof(""));
        sects->text->name         = segment_memcpy(shstrtab, ".text", sizeof(".text"));
        sects->rodata->name       = segment_memcpy(shstrtab, ".rodata", sizeof(".rodata"));
        sects->data->name         = segment_memcpy(shstrtab, ".data", sizeof(".data"));
        sects->bss->name          = segment_memcpy(shstrtab, ".bss", sizeof(".bss"));
        sects->shstrtab->name     = segment_memcpy(shstrtab, ".shstrtab", sizeof(".shstrtab"));
        sects->symtab->name       = segment_memcpy(shstrtab, ".symtab", sizeof(".symtab"));
        sects->strtab->name       = segment_memcpy(shstrtab, ".strtab", sizeof(".strtab"));
        sects->rela_text->name    = segment_memcpy(shstrtab, ".rela.text", sizeof(".rela.text"));   

        return shstrtab;
}

int create_elf64(elf64_sections *sects, elf64_symbols *syms, const char *name)
{
        const size_t n_sections = sizeof(elf64_sections) / sizeof(elf64_section *);

        Elf64_Ehdr *ehdr = elf64_create_ehdr();
        assert(ehdr);

        Elf64_Sym *symtab = elf64_create_symtab(syms, sects);
        assert(symtab);

        Elf64_Shdr *shdrs = elf64_create_shdrs(sects);
        assert(shdrs);

        size_t elf64_size = elf64_align(
                shdrs[n_sections - 1].sh_size   + 
                shdrs[n_sections - 1].sh_offset
        );

        fprintf(stderr, "elf64_size: %lu\n", elf64_size);
        
        mmap_data md = {
                .buf  = nullptr,
                .size = elf64_size,
        };        
        
        mmap_out(&md, name);
        char *pos = md.buf;
        
        memcpy(md.buf, ehdr,  sizeof(Elf64_Shdr));
        memcpy(md.buf + ehdr->e_shoff, shdrs, sizeof(Elf64_Shdr) * n_sections);


        elf64_section **sect = (elf64_section **)sects;
        for (size_t i = 0; i < n_sections; i++) {
                if (sect[i]->data)
                        memcpy(md.buf + shdrs[i].sh_offset, sect[i]->data, shdrs[i].sh_size);
        }
       
        mmap_free(&md);

        free(shdrs);
        
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
                .e_shnum        = ASS_ELF_SHNUM,
                .e_shstrndx     = ASS_ELF_NDX_SHSTRTAB,            
        };

        memcpy(ehdr->e_ident, e_ident, EI_NIDENT);
        return ehdr;
}


static inline size_t elf64_align(size_t addr, size_t align)
{
        return addr + (align - addr) % align; 
}
        



