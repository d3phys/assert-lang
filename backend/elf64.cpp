#include <elf.h>
#include <logs.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iommap.h>
#include <backend/elf64.h>
#include <backend/backend.h>

static Elf64_Ehdr *elf64_create_ehdr();

extern const size_t ASS_ELF_ALIGN;
static inline size_t elf64_align(size_t addr, size_t align = ASS_ELF_ALIGN);


int main()
{
        ac_virt_machine vm = {};
        
        ac_segment text = {};
        const unsigned char text_data[] = {
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

        segment_mmap(&text, (const char *)text_data, sizeof(text_data));
        vm.segment.text = &text;


        ac_segment rodata = {};
        const unsigned char rodata_data[] = {
                0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0xbe, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        segment_mmap(&rodata, (const char *)rodata_data, sizeof(rodata_data));
        vm.segment.rodata = &rodata;


        ac_segment data = {};
        const unsigned char data_data[] = {
                0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x23, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        segment_mmap(&data, (const char *)data_data, sizeof(data_data));
        vm.segment.data = &data;


        ac_segment bss = {};
        bss.size = 0x30;
        vm.segment.bss = &bss;


        ac_symtab st = {};
        st._start = 0xa;
        
        construct_stack(&st.rela_text);

        /* Fill text relocation table */
        Elf64_Rela rela0 = {0x13, ELF64_R_INFO(0x6, R_X86_64_PC32), -0x04}; push_stack(&st.rela_text, &rela0);
        Elf64_Rela rela1 = {0x1b, ELF64_R_INFO(0x5, R_X86_64_32S),   0x00}; push_stack(&st.rela_text, &rela1);
        Elf64_Rela rela2 = {0x25, ELF64_R_INFO(0x6, R_X86_64_PC32), -0x04}; push_stack(&st.rela_text, &rela2);
        Elf64_Rela rela3 = {0x2d, ELF64_R_INFO(0x3, R_X86_64_32S),   0x00}; push_stack(&st.rela_text, &rela3);
        Elf64_Rela rela4 = {0x35, ELF64_R_INFO(0x3, R_X86_64_32S),   0x08}; push_stack(&st.rela_text, &rela4);
        Elf64_Rela rela5 = {0x3d, ELF64_R_INFO(0x4, R_X86_64_32S),   0x00}; push_stack(&st.rela_text, &rela5);
        Elf64_Rela rela6 = {0x45, ELF64_R_INFO(0x4, R_X86_64_32S),   0x08}; push_stack(&st.rela_text, &rela6);
        Elf64_Rela rela7 = {0x57, ELF64_R_INFO(0x5, R_X86_64_32S),   0x10}; push_stack(&st.rela_text, &rela7);

        create_elf64(&vm, &st, "unit");

        destruct_stack(&st.rela_text);
        
        segment_free(&text);
        segment_free(&data);
        segment_free(&rodata);

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

union elf64_sections {
        
};

struct elf64_section {
        ptrdiff_t name = 0;
        Elf64_Shdr *shdr = 0;      
};

struct elf64_sections {
              
};
  
static Elf64_Shdr *elf64_create_shdrs(
        ac_symtab *st, 
        ac_virt_machine *vm,
        ac_segment *shstrtab, 
        ac_segment *strtab
) {
        Elf64_Shdr *shdrs = (Elf64_Shdr *)calloc(ASS_ELF_SHNUM, sizeof(Elf64_Shdr));
        assert(shdrs);
        if (!shdrs)
                return nullptr;

        /* Fill names to know the size of the strtab */
        Elf64_Word names[ASS_ELF_SHNUM] = {0};
        
        names[ASS_ELF_NDX_NULL]      = segment_memcpy(shstrtab, "", sizeof(""));
        names[ASS_ELF_NDX_TEXT]      = segment_memcpy(shstrtab, ".text", sizeof(".text"));
        names[ASS_ELF_NDX_RODATA]    = segment_memcpy(shstrtab, ".rodata", sizeof(".rodata"));
        names[ASS_ELF_NDX_DATA]      = segment_memcpy(shstrtab, ".data", sizeof(".data"));
        names[ASS_ELF_NDX_BSS]       = segment_memcpy(shstrtab, ".bss", sizeof(".bss"));
        names[ASS_ELF_NDX_SHSTRTAB]  = segment_memcpy(shstrtab, ".shstrtab", sizeof(".shstrtab"));
        names[ASS_ELF_NDX_SYMTAB]    = segment_memcpy(shstrtab, ".symtab", sizeof(".symtab"));
        names[ASS_ELF_NDX_STRTAB]    = segment_memcpy(shstrtab, ".strtab", sizeof(".strtab"));
        names[ASS_ELF_NDX_RELA_TEXT] = segment_memcpy(shstrtab, ".rela.text", sizeof(".rela.text"));

        size_t size      = 0;
        Elf64_Off offset = elf64_align(sizeof(Elf64_Ehdr)) + 
                           elf64_align(ASS_ELF_SHNUM * sizeof(Elf64_Shdr));

        offset += elf64_align(size);
        size = vm->segment.text->size;
        shdrs[ASS_ELF_NDX_TEXT] = {
                .sh_name      = names[ASS_ELF_NDX_TEXT],
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
        size = vm->segment.rodata->size;
        shdrs[ASS_ELF_NDX_RODATA] = {
                .sh_name      = names[ASS_ELF_NDX_RODATA],
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
        size = vm->segment.data->size;
        shdrs[ASS_ELF_NDX_DATA] = {
                .sh_name      = names[ASS_ELF_NDX_DATA],
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
        size = vm->segment.bss->size;
        shdrs[ASS_ELF_NDX_BSS] = {
                .sh_name      = names[ASS_ELF_NDX_BSS],
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

        size = shstrtab->size;
        shdrs[ASS_ELF_NDX_SHSTRTAB] = {
                .sh_name      = names[ASS_ELF_NDX_SHSTRTAB],
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
        size = sizeof(Elf64_Sym) * ASS_ELF_SYMTABNUM;
        shdrs[ASS_ELF_NDX_SYMTAB] = {
                .sh_name      = names[ASS_ELF_NDX_SYMTAB],
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
        size = strtab->size;
        shdrs[ASS_ELF_NDX_STRTAB] = {
                .sh_name      = names[ASS_ELF_NDX_STRTAB],
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
        size = st->rela_text.size * sizeof(Elf64_Rela);
        shdrs[ASS_ELF_NDX_RELA_TEXT] = {
                .sh_name      = names[ASS_ELF_NDX_RELA_TEXT],
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
     
Elf64_Sym *elf64_create_symtab(ac_symtab *st, ac_segment *strtab, const char *file_name)
{
        Elf64_Sym *symtab = (Elf64_Sym *)calloc(ASS_ELF_SYMTABNUM, sizeof(Elf64_Sym));
        assert(symtab);
        if (!symtab)
                return nullptr;

        segment_memcpy(strtab, "", sizeof(""));
        
        symtab[ASS_SYM_NDX_FILE] =  {
                .st_name        = strtab->size,
                .st_info        = ELF64_ST_INFO(STB_LOCAL, STT_FILE),
                .st_other       = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx       = SHN_ABS,    
                .st_value       = 0,
                .st_size        = 0,  
        };
        
        segment_memcpy(strtab, file_name, strlen(file_name) + 1);

        symtab[ASS_SYM_NDX_TEXT] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = ASS_ELF_NDX_TEXT,    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[ASS_SYM_NDX_RODATA] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = ASS_ELF_NDX_RODATA,    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[ASS_SYM_NDX_DATA] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = ASS_ELF_NDX_DATA,    
                .st_value = 0,
                .st_size  = 0,  
        };
        
        symtab[ASS_SYM_NDX_BSS] = {
                .st_name  = 0,
                .st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = ASS_ELF_NDX_BSS,    
                .st_value = 0,
                .st_size  = 0,  
        };

        symtab[ASS_ELF_LOCALSNUM] = {
                .st_name  = strtab->size,
                .st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = 0,    
                .st_value = 0,
                .st_size  = 0,  
        };

        segment_memcpy(strtab, "print", sizeof("print"));

        /* _start */
        symtab[ASS_ELF_SYMTABNUM - 1] = {
                .st_name  = strtab->size,
                .st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE),
                .st_other = ELF64_ST_VISIBILITY(STV_DEFAULT),
                .st_shndx = ASS_ELF_NDX_TEXT,    
                .st_value = st->_start,
                .st_size  = 0,  
        };

        segment_memcpy(strtab, "_start", sizeof("_start"));

        return symtab;
}

int create_elf64(ac_virt_machine *vm, ac_symtab *st, const char *name)
{
        Elf64_Ehdr *ehdr = elf64_create_ehdr();
        assert(ehdr);

        ac_segment strtab = {};
        Elf64_Sym *symtab = elf64_create_symtab(st, &strtab, name);
        assert(symtab);

        ac_segment shstrtab = {};
        Elf64_Shdr *shdrs = elf64_create_shdrs(st, vm, &shstrtab, &strtab);
        assert(shdrs);

        size_t elf64_size = elf64_align(shdrs[ASS_ELF_SHNUM - 1].sh_size + 
                                        shdrs[ASS_ELF_SHNUM - 1].sh_offset);

        fprintf(stderr, "elf64_size: %lu\n", elf64_size);
        
        mmap_data md = {
                .buf  = nullptr,
                .size = elf64_size,
        };        
        
        mmap_out(&md, name);
        char *pos = md.buf;
        
        memcpy(md.buf, ehdr,  sizeof(Elf64_Shdr));
        memcpy(md.buf + ehdr->e_shoff, shdrs, sizeof(Elf64_Shdr) * ASS_ELF_SHNUM);
        memcpy(md.buf + shdrs[ASS_ELF_NDX_SYMTAB].sh_offset, symtab, shdrs[ASS_ELF_NDX_SYMTAB].sh_size);
        memcpy(md.buf + shdrs[ASS_ELF_NDX_STRTAB].sh_offset, strtab.data, shdrs[ASS_ELF_NDX_STRTAB].sh_size);
        memcpy(md.buf + shdrs[ASS_ELF_NDX_SHSTRTAB].sh_offset, shstrtab.data, shdrs[ASS_ELF_NDX_SHSTRTAB].sh_size);
        memcpy(md.buf + shdrs[ASS_ELF_NDX_TEXT].sh_offset, vm->segment.text->data, shdrs[ASS_ELF_NDX_TEXT].sh_size);
        memcpy(md.buf + shdrs[ASS_ELF_NDX_RODATA].sh_offset, vm->segment.rodata->data, shdrs[ASS_ELF_NDX_RODATA].sh_size);
        memcpy(md.buf + shdrs[ASS_ELF_NDX_DATA].sh_offset, vm->segment.data->data, shdrs[ASS_ELF_NDX_DATA].sh_size);

        Elf64_Rela *rela = (Elf64_Rela *)(md.buf + shdrs[ASS_ELF_NDX_RELA_TEXT].sh_offset);
        for (size_t i = 0; i < st->rela_text.size; i++) {
                rela[i] = *(Elf64_Rela *)st->rela_text.items[i];
        }
        
        //memcpy(md.buf + shdrs[ASS_ELF_NDX_RELA_TEXT].sh_offset, st->rela_text.items, shdrs[ASS_ELF_NDX_RELA_TEXT].sh_size);
        
        mmap_free(&md);

        free(shdrs);
        segment_free(&shstrtab);
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
        



