#include <stdio.h>
#include <stdlib.h>
#include <logs.h>
#include <array.h>
#include <iommap.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stack.h>
#include <ast/tree.h>
#include <ast/keyword.h>
#include <backend/backend.h>
#include <backend/elf64.h>

static int input_error();
static int file_error(const char *file_name);


int main(int argc, char *argv[])
{
        if (argc != 3) {
                fprintf(stderr, ascii(RED, "There must be 2 arguments\n"));
                return EXIT_FAILURE;
        }

        const char *src_file = argv[1];
        const char *out_file = argv[2];

        clock_t start = clock();
        mmap_data md = {0};
        int error = mmap_in(&md, src_file);
        if (error)
                return EXIT_FAILURE;

        array idents = {0};

        ast_node *err = nullptr;

        elf64_section *secs = (elf64_section *)calloc(SEC_NUM, sizeof(elf64_section));
        assert(secs);

        elf64_symbol *syms = (elf64_symbol *)calloc(SYM_NUM, sizeof(elf64_symbol));
        assert(syms);

        char *reader = md.buf;
        ast_node *tree = read_ast_tree(&reader, &idents);
        mmap_free(&md);
        if (!tree)
                goto fail;

        fill_sections_names(secs);
        fill_symbols_info(secs, syms, out_file);

        $(dump_tree(tree);)
        err = compile_tree(tree, secs, syms);
        if (err)
                goto fail;

        create_elf64(secs, syms, out_file);
        
        for (size_t i = 0; i < SEC_NUM; i++)
                if (secs[i].data)
                        section_free(secs + i);
        
        free(syms);
        free(secs);
        
fail:
        char **data = (char **)idents.data;
        for (size_t i = 0; i < idents.size; i++) {
                free(data[i]);
        }

        free_array(&idents, sizeof(char *));

        clock_t end = clock();

        if (!tree || error) {
                fprintf(stderr, ascii(RED, "Compilation failed\n"));
                return EXIT_FAILURE;
        }

        fprintf(stderr, ascii(GREEN, "Compilation succeed: %lf sec\n"), (end - start) / CLOCKS_PER_SEC);
        return EXIT_SUCCESS;
}

static int input_error()
{
        fprintf(stderr, ascii(RED, "There must be 3 arguments\n"));
        return EXIT_FAILURE;
}

static int file_error(const char *file_name)
{
        fprintf(stderr, ascii(RED, "Can't open file %s: %s\n"), 
                        file_name, strerror(errno));

        return EXIT_FAILURE;
}


