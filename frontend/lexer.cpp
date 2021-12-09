#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <logs.h>
#include <list.h>
#include <array.h>
#include <frontend/token.h>
#include <frontend/tree.h>
#include <frontend/grammar.h>

static token *get_ident(array *const toks, const char **str, array *const vars);
static token *get_number(array *const toks, const char **str);

static token *create_token(array *const toks, int type) 
{
        assert(toks);

        token *newbie = (token *)array_create(toks, sizeof(token));

        newbie->type       = type;
        newbie->data.ident = nullptr;

        return newbie;
}

static const char *create_var(array *const variables, const char *str, const size_t len)
{
        assert(str);

        char **vars = (char **)variables->data;
        for (size_t i = 0; i < variables->size; i++) {
                if (strlen(vars[i]) == len && !strncmp(vars[i], str, len)) 
                        return vars[i];
        }

        char *newbie = (char *)calloc(len + 1, sizeof(char));
        if (!newbie) {
                perror("Failed to allocate variable name");
                return nullptr;
        }

        strncpy(newbie, str, len);
        newbie[len] = '\0';

        array_push(variables, &newbie, sizeof(char *));

        return newbie;
}

token *tokenize(const char *str, array *const variables)
{
        assert(str);
        assert(variables);

#define T(id)                                    \
        tok = create_token(&arr, TOKEN_KEYWORD); \
        tok->data.keyword = id;

        array arr  = {0};
        array vars = {0};

        token *tok = nullptr;
        while (*str != '\0') {
                switch (*str) {
                case ' ':
                case '\t':
                case '\n':
                        break;
                case '=':
                        if (*(str+1) == '=') {
                                T(KW_EQUAL);
                                str++;
                        } else {
                                T(KW_ASSIGN);
                        }
                        break;
                case '>':
                        if (*(str+1) == '=') {
                                T(KW_GEQUAL);
                                str++;
                        } else {
                                T(KW_GREAT);
                        }
                        break;
                case '<':
                        if (*(str+1) == '=') {
                                T(KW_LEQUAL);
                                str++;
                        } else {
                                T(KW_LOW);
                        }
                        break;
                case '!':
                        if (*(str+1) != '=')
                                goto syntax_error;

                        T(KW_NEQUAL);
                        str++;
                        break;
                case '|':
                        T(KW_OR);
                        break;
                case '&':
                        T(KW_AND);
                        break;
                case '(':
                        T(KW_OPAR);
                        break;
                case ')':
                        T(KW_CPAR);
                        break;
                case '+':
                        T(KW_ADD);
                        break;
                case '-':
                        T(KW_SUB);
                        break;
                case '*':
                        T(KW_MUL);
                        break;
                case '/':
                        T(KW_DIV);
                        break;
                case '{':
                        T(KW_BEGIN);
                        break;
                case '}':
                        T(KW_END);
                        break;
                case ';':
                        T(KW_STEND);
                        break;
                case '^':
                        T(KW_POW);
                        break;
                case '0'...'9':
                        get_number(&arr, &str);
                        str--;
                        break;
                default:
                        get_ident(&arr, &str, &vars);
                        str--;
                        break;
                }

                str++;
        }

        T(KW_STOP);
        
#undef T 

syntax_error:
        printf("ERROR:%s\n", str);

        token *tokens = (token *)array_extract(&arr, sizeof(token));
        token *iter   = tokens;

        dump_array(&vars, sizeof(char *), array_string);
        dump_tokens(iter);
        free(tokens);

        return 0;
}

static token *get_ident(array *const toks, const char **str, array *const vars)
{
        assert(str);
        assert(toks);

        const char *start = *str;


#define cmp(tok)  !strncmp(start, keyword_ident(tok), (size_t)(*str - start))

#define proc(id)                                      \
        token* t = create_token(toks, TOKEN_KEYWORD); \
        t->data.keyword = id;                         \
        return t;

        while (**str != '\0') {
                switch (**str) {
                case '\t':
                case '\n':
                case ' ':
                case '=':
                case '>':
                case '<':
                case '!':
                case '&':
                case '(':
                case ')':
                case '+':
                case '-':
                case '*':
                case '/':
                case '{':
                case '}':
                case ';':
                             if (cmp(KW_IF))     { proc(KW_IF);     }
                        else if (cmp(KW_ELSE))   { proc(KW_ELSE);   }
                        else if (cmp(KW_WHILE))  { proc(KW_WHILE);  }
                        else if (cmp(KW_CONST))  { proc(KW_CONST);  }
                        else if (cmp(KW_DEFINE)) { proc(KW_DEFINE); }
                        else if (cmp(KW_RETURN)) { proc(KW_RETURN); }
                        else { 
                                token *newbie = create_token(toks, TOKEN_IDENT);
                                newbie->data.ident = create_var(vars, start, (size_t)(*str - start));
                                return newbie;
                        }
                        break;
                default:
                        (*str)++;
                        break;
                }
        }

#undef cmp
#undef proc 

        return nullptr;
}

static token *get_number(array *const toks, const char **str)
{
        assert(str && *str);
        assert(toks);

        double num = 0;
        int n      = 0;

        sscanf(*str, "%lf%n", &num, &n);
        token *newbie = create_token(toks, TOKEN_NUMBER);
        newbie->data.number = num;

        *str += n;

        return newbie;
}

int main()
{
        array variables = {0};

        int a = 3;
        const char *str = "if {}{}{}канстанта если (канстанта ewew111>===0 | y!=2){const hel111lo3=-2.21e12; y = x * 32 ^ result; if (zero < 2 & keyval != 2.2212*321) -0.321e-1 - 1.22 Gar1k; while (result <= 5.0) 32;}";
        printf("\n%s\n", str);

        tokenize(str, &variables);
        return 0;
}

void dump_tokens(const token *toks)
{
        assert(toks);
        
        size_t line = 0;
        fprintf(logs, "Tokens dump:\n\n");
        fprintf(logs, "%s", "================================================\n"
                            "| <b>Types</b>                                         |\n"
                            "================================================\n");
        do {
                fprintf(logs, "------------------------------------------------\n"
                              "| %-4lu | ", line++);

                if (toks->type == TOKEN_KEYWORD) {
                        fprintf(logs, "<font color=\"blue\">keyword</font> | %s [%d or '%c']\n", 
                                        keyword_ident(toks->data.keyword), 
                                        toks->data.keyword, toks->data.keyword);
                } else if (toks->type == TOKEN_NUMBER) {
                        fprintf(logs, "<font color=#ca6f1e>number</font>  | %lg\n", toks->data.number);
                } else if (toks->type == TOKEN_IDENT) {
                        fprintf(logs, "<font color=\"green\">ident</font>   | %s [%p]\n", toks->data.ident, toks->data.ident);
                } else {
                        fprintf(logs, "<font color=\"red\">ERROR</font>\n");
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








