#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <logs.h>
#include <array.h>
#include <errno.h>
#include <iommap.h>

#include <ast/tree.h>
#include <frontend/token.h>
#include <frontend/compile.h>

static int input_error();
static int file_error(const char *file_name);

int main(int argc, char *argv[])
{
        if (argc != 3)
                return input_error();

        const char *src_file  = argv[1];
        const char *tree_file = argv[2];

        FILE *out = fopen(tree_file, "w");
        if (!out)
                return file_error(tree_file);

        mmap_data md = {0};
        int error = mmap_in(&md, src_file);
        if (error)
                return EXIT_FAILURE;

        array names = {0};

        token *toks = tokenize(md.buf, &names);
        dump_tokens(toks);
        fprintf(stderr, ascii(blue, "Tokens created --- %s\n"), local_time("%H:%M:%S"));
        mmap_free(&md);

$       (dump_tokens(toks);)
$       (dump_array(&names, sizeof(char *), array_string);)

        token *iter = toks;
        ast_node *tree = grammar_rule(&iter);
        fprintf(stderr, ascii(blue, "Tree created   --- %s\n"), local_time("%H:%M:%S"));

        if (!tree) {
                free(toks);
                char **data = (char **)names.data;
                for (size_t i = 0; i < names.size; i++) {
                        free(data[i]);
                }

                free_array(&names, sizeof(char *));

                fprintf(stderr, ascii(red, "..................\n"
                                           "Compilation failed\n"));
                return EXIT_FAILURE;
        }

        if (tree) {
                $(dump_tree(tree);)
        }

        save_ast_tree(out, tree);
        fprintf(stderr, ascii(blue, "Tree saved     --- %s\n"), local_time("%H:%M:%S"));
        free(toks);

        char **data = (char **)names.data;
        for (size_t i = 0; i < names.size; i++) {
                free(data[i]);
        }

        free_array(&names, sizeof(char *));
        fclose(out);

        fprintf(stderr, ascii(green, "Abstract syntax tree compiled --- %s\n"), local_time("%x %H:%M:%S"));
        return EXIT_SUCCESS;
}

static int input_error()
{
        fprintf(stderr, ascii(red, "There must be 3 arguments\n"));
        return EXIT_FAILURE;
}

static int file_error(const char *file_name)
{
        fprintf(stderr, ascii(red, "Can't open file %s: %s\n"), 
                        file_name, strerror(errno));

        return EXIT_FAILURE;
}


