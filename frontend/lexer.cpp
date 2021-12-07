#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <logs.h>
#include <stack.h>
#include <frontend/token.h>
#include <frontend/tree.h>
#include <frontend/grammar.h>
#include <frontend/lexer.h>

struct array {
        size_t capacity = 0;
        size_t size     = 0;

        token *data = nullptr;
};

static const size_t INIT_CAPACITY = 2;

static token *realloc_array(array *const toks, size_t capacity) 
{
        if (capacity == 0)
                capacity = INIT_CAPACITY;

        token *data = (token *)realloc(toks->data, capacity * sizeof(token));
        if (!data) {
                perror("Can't realloc token array");
                return nullptr;
        }

        toks->capacity = capacity;
        toks->data     = data;

        return data;
}

static token *create_token(array *const toks, int type) 
{
        assert(toks);

        if (toks->size >= toks->capacity) {
                token *data = realloc_array(toks, toks->capacity * 2);
                if (!data)
                        return nullptr;
        }

        token *newbie = &toks->data[toks->size++];

        newbie->type       = type;
        newbie->data.ident = nullptr;

        return newbie;
}

static token *extract_tokens(array *const toks) 
{
        assert(toks);

        token *data = realloc_array(toks, toks->size);
        if (!data)
                return nullptr;

        toks->data = nullptr;
        return data;
}

static char *unique(stack *const stk, const char *str, const size_t len)
{
        assert(str);

        char **vars = (char **)data_stack(stk);

        for (size_t i = 0; i < stk->size; i++) {
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

        push_stack(stk, newbie);

        return newbie;
}

token *get_ident   (array *const toks, const char **str, stack *const vars);
token *get_number  (array *const toks, const char **str);
token *get_operator(array *const toks, const char **str);

int tokenize(const char *str)
{
#define T(id)                                    \
        tok = create_token(&arr, TOKEN_KEYWORD); \
        tok->data.keyword = id;

        assert(str);

        stack vars = {0};
        construct_stack(&vars);
        array arr = {0};
        arr.capacity = 0;
        arr.size     = 0;

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
                case 'a'...'z':
                case 'A'...'Z':
                case '_':
                        get_ident(&arr, &str, &vars);
                        str--;
                        break;
                default:
                        goto syntax_error;
                }

                str++;
        }

        T(KW_STOP);
        
#undef T 

syntax_error:
        printf("ERROR:%s\n", str);

        token *tokens = extract_tokens(&arr);
        token *iter   = tokens;
        do {
                if (iter->type == TOKEN_KEYWORD)
                        printf("keyword: %s\n", keyword_ident(iter->data.keyword));
                else if (iter->type == TOKEN_NUMBER)
                        printf("number: %lg\n", iter->data.number);
                else if (iter->type == TOKEN_IDENT)
                        printf("ident: %s\n", iter->data.ident);
                else 
                        printf("error\n ");

                iter++;
        } while (iter->data.keyword != KW_STOP);


        free(tokens);

        dump_stack(&vars);
        while (vars.size > 0) {
                free(pop_stack(&vars));
        }

        destruct_stack(&vars);
        return 0;
}

token *get_ident(array *const toks, const char **str, stack *const vars)
{
        assert(str);
        assert(toks);

        const char *start = *str;


#define cmp(tok)  !strncmp(start, keyword_ident(tok), *str - start)

#define proc(id)                                      \
        token* t = create_token(toks, TOKEN_KEYWORD); \
        t->data.keyword = id;                         \
        return t;

        while (**str != '\0') {
                switch (**str) {
                case '0'...'9':
                case 'a'...'z':
                case 'A'...'Z':
                case '_':
                        (*str)++;
                        break;
                default:
                             if (cmp(KW_IF))     { proc(KW_IF);     }
                        else if (cmp(KW_ELSE))   { proc(KW_ELSE);   }
                        else if (cmp(KW_WHILE))  { proc(KW_WHILE);  }
                        else if (cmp(KW_CONST))  { proc(KW_CONST);  }
                        else if (cmp(KW_DEFINE)) { proc(KW_DEFINE); }
                        else if (cmp(KW_RETURN)) { proc(KW_RETURN); }
                        else { 
                                token *newbie = create_token(toks, TOKEN_IDENT);
                                newbie->data.ident = unique(vars, start, *str - start);
                                return newbie;
                        }
                }
        }

#undef cmp
#undef proc 
}

token *get_number(array *const toks, const char **str)
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
        const char *str = "if(x11 ewew111>===0 | y!=2){const hel111lo3=-2.21e12; y = x * 32 ^ result; if (zero < 2 & keyval != 2.2212*321) -0.321e-1 - 1.22 Gar1k; while (result <= 5.0) 32;}";
        printf("\n%s\n", str);
        tokenize(str);
        return 0;
}




