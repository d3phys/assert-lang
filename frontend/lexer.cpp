#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <logs.h>
#include <list.h>
#include <array.h>
#include <ast/tree.h>

#include <frontend/token.h>
#include <frontend/keyword.h>

static token *lexer_error(const char *str) { return nullptr; }
static token  *core_error() { return nullptr; }

static token *create_keyword(array *const tokens, int keyword);
static token *create_number (array *const tokens, const char **str);
static token *create_ident(array *const tokens, const char *str, 
                           array *const idents, const size_t len);

static token *read_keyword(array *const tokens, const char *str, 
                           array *const idents, size_t length);

token *tokenize(const char *str, array *const idents)
{
        assert(str);
        assert(idents);

        array tokens = {0};

        const char *start = str;

        bool comment = false;
        while (*str != '\0') {
                if (*str == '#')
                        comment = !comment;

                if (comment) {
                        str++;
                        continue;
                }

                if (isspace(*str)) {
                        if (start != str) {
                                read_keyword(&tokens, start, 
                                              idents, (size_t)(str - start));
                        }
                        start = ++str;
                        continue;
                }

#define LINKABLE(xxx)
#define UNLINKABLE(xxx) xxx
#define KEYWORD(name, keyword, ident)                                          \
                else if (!strncmp(ident, str, sizeof(ident) - 1)) {            \
                        if (start != str)                                      \
                                read_keyword(&tokens, start,                   \
                                              idents, (size_t)(str - start));  \
                        create_keyword(&tokens, keyword);                      \
                        str += sizeof(ident) - 1;                              \
                        start = str;                                           \
                        continue;                                              \
                }

                if (0) {} 
#include "../KEYWORDS"
#undef LINKABLE
#undef UNLINKABLE 
#undef KEYWORD 

                if (isdigit(*str) && start == str) {
                        create_number(&tokens, &str);
                        start = str;
                        continue;
                }

                str++;
        }

        create_keyword(&tokens, KW_STOP);

        token *toks = (token *)array_extract(&tokens, sizeof(token));
        if (!toks)
                return core_error();

        return toks;
}

static token *read_keyword(array *const tokens, const char *str, 
                           array *const idents, size_t length) 
{
        assert(tokens);
        assert(idents);
        assert(str);


#define LINKABLE(xxx) xxx
#define UNLINKABLE(xxx) 
#define KEYWORD(name, keyword, ident)                                             \
                if (sizeof(ident) - 1 == length) {                                \
                        if (!strncmp(ident, str, length)) {                       \
                                token *newbie = create_keyword(tokens, keyword);  \
                                if (!newbie)                                      \
                                        return core_error();                      \
                                                                                  \
                                return newbie;                                    \
                        }                                                         \
                }

                if (0) {} 
#include "../KEYWORDS"
#undef LINKABLE
#undef UNLINKABLE 
#undef KEYWORD 

        token *ident = create_ident(tokens, str, idents, length);
        if (!ident)
                return core_error();

        return ident;
}

static token *create_token(array *const tokens, int type) 
{
        assert(tokens);

        token *newbie = (token *)array_create(tokens, sizeof(token));

        newbie->type       = type;
        newbie->data.ident = nullptr;

        return newbie;
}

static token *create_keyword(array *const tokens, int keyword)
{
        assert(tokens);

        token *newbie = create_token(tokens, TOKEN_KEYWORD);
        if (!newbie)
                return core_error();

        newbie->data.keyword = keyword;

        return newbie;
}

static token *create_ident(array *const tokens, const char *str, array *const idents, const size_t len)
{
        assert(str);
        assert(tokens);
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

        token *newbie = create_token(tokens, TOKEN_IDENT);
        if (!newbie)
                return core_error();

        newbie->data.ident = ident;

        return newbie;
}

static token *create_number(array *const tokens, const char **str)
{
        assert(str && *str);
        assert(tokens);

        num_t number = 0;
        int n_skip = 0;

        sscanf(*str, "%ld%n", &number, &n_skip);
        token *newbie = create_token(tokens, TOKEN_NUMBER);
        if (!newbie)
                return core_error();

        newbie->data.number = number;

        *str += n_skip;

        return newbie;
}

void dump_tokens(const token *toks)
{
        assert(toks);
        
        size_t total = 0;
        const token *iter = toks;
        while ((iter++)->data.keyword != KW_STOP)
                total++;

        size_t line = 0;
        fprintf(logs, "Tokens dump:\n\n");
        fprintf(logs, "%s", "================================================\n"
                            "| <b>Types</b>                                         |\n"
                            "================================================\n");
        do {
                fprintf(logs, "+----+----+---------+--------------------------\n"
                              "|%-3lu |%-3lu | ", line, total - line);
                line++;

                if (toks->type == TOKEN_KEYWORD) {
                        fprintf(logs, html(blue, "keyword") " | %s [%d or '%c']\n", 
                                        keyword_string(toks->data.keyword), 
                                        toks->data.keyword, toks->data.keyword);
                } else if (toks->type == TOKEN_NUMBER) {
                        fprintf(logs, html(#ca6f1e, "number") "  | %ld\n", toks->data.number);
                } else if (toks->type == TOKEN_IDENT) {
                        fprintf(logs, html(green, "ident") "   | %s [%p]\n", toks->data.ident, toks->data.ident);
                } else {
                        fprintf(logs, html(red, "ERROR\n"));
                }

                if (toks->data.keyword == KW_STOP)
                        break;

                toks++;
        } while (true);

        fprintf(logs, "================================================\n"
                      "| Total size: %lu x %lu = %lu bytes             \n"
                      "| Address: %p                                   \n"
                      "================================================\n\n\n", line, sizeof(token), line * sizeof(token), toks);
}

