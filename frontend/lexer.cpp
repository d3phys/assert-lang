#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <logs.h>
#include <stack.h>
#include <frontend/tree.h>
#include <frontend/grammar.h>
#include <frontend/lexer.h>


static node *create_token(stack *const stk, int type) 
{
        assert(stk);

        node *newbie = (node *)calloc(1, sizeof(node));

        newbie->data.variable = nullptr;
        newbie->type          = type;
        newbie->left          = nullptr;
        newbie->right         = nullptr;

        push_stack(stk, newbie); 
        return newbie;
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

node *get_ident(stack *const stk, const char **str, stack *const vars);
node *get_number  (stack *const stk, const char **str);
node *get_operator(stack *const stk, const char **str);

int tokenize(const char *str)
{
        assert(str);
        stack stk = {0};
        construct_stack(&stk);

        stack vars = {0};
        construct_stack(&vars);

        node *tok = nullptr;

        while (*str != '\0') {
                switch (*str) {
                case ' ':
                case '\t':
                case '\n':
                        break;
                case '=':
                        if (*(str+1) == '=') {
                                create_token(&stk, EQ_OP);
                                str++;
                        } else {
                                create_token(&stk, '=');
                        }
                        break;
                case '>':
                        if (*(str+1) == '=') {
                                create_token(&stk, GE_OP);
                                str++;
                        } else {
                                create_token(&stk, '>');
                        }
                        break;
                case '<':
                        if (*(str+1) == '=') {
                                create_token(&stk, LE_OP);
                                str++;
                        } else {
                                create_token(&stk, '<');
                        }
                        break;
                case '!':
                        if (*(str+1) != '=')
                                goto syntax_error;

                        create_token(&stk, NEQ_OP);
                        str++;
                        break;
                case '|':
                        create_token(&stk, OR_OP);
                        break;
                case '&':
                        create_token(&stk, AND_OP);
                        break;
                case '(':
                case ')':
                case '+':
                case '-':
                case '*':
                case '/':
                case '{':
                case '}':
                case ';':
                case '^':
                        create_token(&stk, *str);
                        break;
                case '0'...'9':
                        get_number(&stk, &str);
                        str--;
                        break;
                case 'a'...'z':
                case 'A'...'Z':
                case '_':
                        get_ident(&stk, &str, &vars);
                        str--;
                        break;
                default:
                        goto syntax_error;
                }

                str++;
        }

syntax_error:
        printf("ERROR:%s\n", str);


        for (ptrdiff_t i = 0; i < stk.size - 1; i++) {
                ((node *)data_stack(&stk)[i])->left = (node *)data_stack(&stk)[i + 1];
        }

        dump_tree((node *)*data_stack(&stk));
        free_tree((node *)*data_stack(&stk));
        destruct_stack(&stk);

        dump_stack(&vars);
        while (vars.size > 0) {
                free(pop_stack(&vars));
        }

        destruct_stack(&vars);
        return 0;
}

node *get_ident(stack *const stk, const char **str, stack *const vars)
{
        assert(str);
        assert(stk);

        const char *token = *str;

#define cmp(tok)  !strncmp(token, tok, *str - token)
#define proc(id) \
        printf ("READ IDENT:%c, %d\n", id, id); \
        return create_token(stk, id);

        while (**str != '\0') {
                switch (**str) {
                case '0'...'9':
                case 'a'...'z':
                case 'A'...'Z':
                case '_':
                        (*str)++;
                        break;
                default:
                             if (cmp("if"))    { proc(IF);    }
                        else if (cmp("else"))  { proc(ELSE);  }
                        else if (cmp("while")) { proc(WHILE); }
                        else if (cmp("const")) { proc(CONST); }
                        else { 
                                node *tok = create_token(stk, VARIABLE);
                                tok->data.variable = unique(vars, token, *str - token);
                                //tok->data.variable = "x";
                                return tok;
                        }
                }
        }

#undef cmp
#undef proc 
}

node *get_number(stack *const stk, const char **str)
{
        assert(str && *str);
        assert(stk);

        double num = 0;
        int n      = 0;

        sscanf(*str, "%lf%n", &num, &n);
        node *newbie = create_token(stk, NUMBER);
        newbie->data.number = num;

        *str += n;

        return newbie;
}

int main()
{
        const char *str = "if(x11 >===0||y!=2){const hel111lo3=-2.21e12; y = x * 32 ^ result; if (zero < 2 & keyval != 2.2212*321) -0.321e-1 - 1.22 Gar1k; while (result <= 5.0) 32;}";
        tokenize(str);
        return 0;
}




