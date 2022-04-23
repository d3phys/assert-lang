#include <stdio.h>
#include <string.h>
#include <iommap.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <logs.h>
#include <array.h>

#include <ast/tree.h>
#include <ast/keyword.h>

static ast_node *syntax_error(char *str);
static ast_node *core_error();

static ast_node *read_ast_node(char **str, array *const idents);

static ast_node *read_keyword(char *str, size_t length);
static ast_node *read_data(char **str, array *const idents);

static ast_node *create_ident(const char *str, array *const idents, const size_t len);

static char *find_bracket(char *str);
static char *rfind(char *str, char ch);

static inline void skip_spaces(char **str);
static inline void move(char **str);
static inline char cur(char **str);

ast_node *read_ast_tree(char **str, array *const idents)
{
        assert(str);
        assert(idents);

        ast_node *tree = read_ast_node(str, idents);
        if (!tree)
                return syntax_error(*str);

        if (**str != '\0')
                return syntax_error(*str);

        return tree;
}

static ast_node *read_ast_node(char **str, array *const idents)
{
        assert(str);
        assert(idents);

        skip_spaces(str);
        if (cur(str) != '(')
                return nullptr;

        move(str);
        skip_spaces(str);

        ast_node *left = nullptr;
        if (cur(str) == '(') {
                left = read_ast_node(str, idents);
                if (!left)
                        return nullptr;
        }

        ast_node *root = read_data(str, idents);
        if (!root)
                return nullptr;

        skip_spaces(str);
        ast_node *right = nullptr;
        if (cur(str) == '(') {
                right = read_ast_node(str, idents);
                if (!right)
                        return nullptr;
        }

        root->left  = left;
        root->right = right;

        move(str);
        skip_spaces(str);
        return root;
}
/*
int main() 
{
        mmap_data md = {0};
        int err = mmap_in(&md, "tree"); 
        if (err)
                return 0;

        char *r = md.buf;
        array idents = {0};
        ast_node *rt = read_ast_tree(&r, &idents);
        if (rt)
                dump_tree(rt);

        mmap_free(&md);
        char **data = (char **)idents.data;
        for (size_t i = 0; i < idents.size; i++) {
                free(data[i]);
        }

        free_array(&idents, sizeof(char *));

        return 0;
}*/

static ast_node *read_keyword(char *str, size_t length) 
{
        assert(str);

#define AST(name, keyword, ident)                                                 \
                if (sizeof(ident) - 1 == length) {                                \
                        if (!strncmp(ident, str, length)) {                       \
                                ast_node *newbie = create_ast_keyword(keyword);   \
                                if (!newbie)                                      \
                                        return core_error();                      \
                                                                                  \
                                return newbie;                                    \
                        }                                                         \
                }

                if (0) {} 
#include "../AST"

#undef AST 

        return syntax_error(str);
}

static ast_node *syntax_error(char *str)
{
        fprintf(stderr, ascii(RED, "Syntax error: %s"), str);
        return nullptr;
}

static ast_node *core_error()
{
        fprintf(stderr, ascii(RED, "Core error"));
        return nullptr;
}

static ast_node *create_ident(const char *str, array *const idents, const size_t len)
{
        assert(str);
        assert(idents);

        char *ident = nullptr;
        char **keys = (char **)idents->data;
        for (size_t i = 0; i < idents->size; i++) {
                if (strlen(keys[i]) == len && !strncmp(keys[i], str, len)) {
                        ident = keys[i];
                        break;
                }
        }

        if (!ident) {
                ident = (char *)calloc(len + 1, sizeof(char));
                if (!ident)
                        return core_error();

                strncpy(ident, str, len);
                ident[len] = '\0';

                array_push(idents, &ident, sizeof(char *));
        }

        ast_node *root = create_ast_ident(ident);
        if (!root)
                return core_error();

        return root;
}

static ast_node *read_data(char **str, array *const idents)
{
        assert(str);
        assert(idents);

        ast_node *root = nullptr;

        char *end = *str;
        double number = strtod(*str, &end);
        if (end != *str) {
                root = create_ast_number(number);
                if (!root)
                        return core_error();

                *str = end;
                return root;
        }

        skip_spaces(str);
        if (cur(str) == '\'') {
                end = find_bracket(*str);
                end = rfind(end, '\'');
                if (*str == end)
                        return syntax_error(*str);

                move(str);
                root = create_ident(*str, idents,(size_t)(end - *str));
                if (!root)
                        return core_error();

                *str = end + 1;
                return root;
        }

        end = find_bracket(*str);
        end--;
        while (isspace(*end))
                end--;

        root = read_keyword(*str, (size_t)(end - *str + 1));
        if (!root)
                return syntax_error(*str);

        *str = end + 1;
        return root;
}

static char *find_bracket(char *str)
{
        assert(str);

        while (*str != '\0' && *str != ')' && *str != '(')
                str++;

        return str;
}

static char *rfind(char *str, char ch)
{
        assert(str);

        while (*str != ch)
                str--;

        return str;
}

static inline void skip_spaces(char **str)
{
        assert(str);

        while (isspace(**str))
                (*str)++;
}

static inline void move(char **str)
{
        assert(str);
        (*str)++;
}

static inline char cur(char **str)
{
        assert(str);
        return (**str);
}

