#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <logs.h>
#include <array.h>
#include <errno.h>
#include <iommap.h>
#include <time.h>

#include <ast/tree.h>
#include <trans/transpile.h>

static int input_error();
static int file_error(const char *file_name);

int main(int argc, char *argv[])
{
        if (argc != 3) {
                fprintf(stderr, ascii(red, "There must be 2 arguments\n"));
                return EXIT_FAILURE;
        }

        const char *src_file = argv[1];
        const char *out_file = argv[2];

        clock_t start = clock();
        FILE *out = fopen(out_file, "w");
        if (!out)
                return file_error(out_file);

        mmap_data md = {0};
        int error = mmap_in(&md, src_file);
        if (error)
                return EXIT_FAILURE;

        array idents = {0};

        ast_node *err = nullptr;
        char *reader = md.buf;
        ast_node *tree = read_ast_tree(&reader, &idents);
        mmap_free(&md);
        if (!tree)
                goto fail;

        err = trans_stmt(out, tree);
        if (error)
                goto fail;

fail:
        char **data = (char **)idents.data;
        for (size_t i = 0; i < idents.size; i++) {
                free(data[i]);
        }

        free_array(&idents, sizeof(char *));
        fclose(out);

        clock_t end = clock();

        if (!tree || error) {
                fprintf(stderr, ascii(red, "Transpilation failed\n"));
                return EXIT_FAILURE;
        }

        fprintf(stderr, ascii(green, "Transpilation succeed: %lf sec\n"), (end - start) / CLOCKS_PER_SEC);
        return EXIT_SUCCESS;
}

static int input_error()
{
        fprintf(stderr, ascii(red, "There must be 2 arguments\n"));
        return EXIT_FAILURE;
}

static int file_error(const char *file_name)
{
        fprintf(stderr, ascii(red, "Can't open file %s: %s\n"), 
                        file_name, strerror(errno));

        return EXIT_FAILURE;
}


