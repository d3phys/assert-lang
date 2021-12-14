#ifndef TOKEN_H
#define TOKEN_H

#include <array.h>

enum token_type {
        TOKEN_KEYWORD = 0x01,
        TOKEN_IDENT   = 0x02,
        TOKEN_NUMBER  = 0x03,
};

struct token {
        int type = 0;

        union {
                const char *ident;
                double     number;
                int       keyword;
        } data;
};

token *tokenize(const char *str, array *const variables);
void dump_tokens(const token *toks);



#endif /* TOKEN_H */
