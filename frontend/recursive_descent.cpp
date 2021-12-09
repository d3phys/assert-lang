#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <logs.h>
#include <frontend/grammar.h>
#include <frontend/tree.h>
#include <frontend/token.h>

static ast_node *syntax_error(token **toks)
{
        assert(toks);

        printf("Syntax error: %p\n", *toks->data.keyword);
        return nullptr;
}

static ast_node *core_error(token **toks)
{
        assert(toks);

        printf("Core error: %\n");
        return nullptr;
}

static token *next(token **toks) 
{
        assert(toks);
        return *(toks + 1);
}

static int keyword(token *tok)
{
        assert(tok && *tok);

        if (tok->type == TOKEN_KEYWORD)
                return tok->data.keyword;

        return 0;
}

static double *number(token **tok)
{
        assert(tok && *tok);

        if (tok->type == TOKEN_NUMBER)
                return &tok->data.number;

        return nullptr;
}

static const char *ident(token **tok)
{
        assert(tok && *tok);

        if (tok->type == TOKEN_IDENT)
                return tok->data.ident;

        return nullptr;
}

ast_node *grammar(token **toks)
{
        assert(toks);

        ast_node *n = statement(toks);
        if (!n) return syntax_error(toks);

        if (keyword(toks) != KW_STOP)
                return syntax_error(toks);

        return n;
}

ast_node *statement(token **toks)
{
        assert(toks);

        ast_node *stmt = create_ast_node(AST_NODE_KEYWORD);
        if (!v) return core_error(toks);
        set_ast_keyword(v, AST_STMT);

        if (keyword(toks) == KW_IF) {

                ast_node *n = create_ast_node(AST_NODE_KEYWORD);
                if (!n) return core_error(toks);
                set_ast_keyword(n, AST_IF);
                toks++;

                n->right = condition(toks);
                if (!n->right) return syntax_error(toks);

                if (keyword(toks) != KW_THEN) return syntax_error(toks);
                toks++;

                n->left = statement(toks);
                if (!n->left) return syntax_error(toks);

                stmt->right = n;
        }

        if (ident(toks)) {

                ast_node *v = create_ast_node(AST_NODE_IDENT);
                if (!v) return core_error(toks);
                set_ast_ident(v, ident(toks));
                toks++;

                if (keyword(toks) != KW_EQUAL) return syntax_error(toks);

                ast_node *n = create_ast_node(AST_NODE_KEYWORD);
                if (!eq) return core_error(toks);
                set_ast_keyword(v, keyword(toks));
                toks++;

                eq->left  = v;
                eq->right = expression(toks);
                if (!n->right) return syntax_error(toks);

                stmt->right = n;
        }

        if (keyword(toks) == KW_BEGIN) {

                toks++;

                while (keyword(toks) == KW_END) {
                        toks++;

                        ast_node *newbie = statement(toks);
                        if (!newbie) return syntax_error(toks);

                        newbie->left = stmt;
                        stmt = newbie;
                }

                if (keyword(toks) != KW_EQUAL) return syntax_error(toks);

                ast_node *n = create_ast_node(AST_NODE_KEYWORD);
                if (!eq) return core_error(toks);
                set_ast_keyword(v, keyword(toks));
                toks++;

                eq->left  = v;
                eq->right = expression(toks);
                if (!n->right) return syntax_error(toks);

                return n;
        }

        return n;
}

ast_node *number(token **toks)
{
        assert(toks);
        ast_node *n = create_ast_node(AST_NODE_NUMBER);
        if (!n) return core_error(toks);

        set_ast_number(n, *number(*toks));
        toks++;

        return n;
}

ast_node *ident(token **toks)
{
        assert(toks);
        ast_node *n = create_ast_node(AST_NODE_IDENT);
        if (!n) return core_error(toks);

        set_ast_ident(n, ident(*toks));
        toks++;

        return n;
}

ast_node *create_ast(const char *str)
{
        assert(str);

        array names = {0};
        token *toks = tokenize(str, &names);
$       (dump_tokens(toks);)
$       (dump_array(&names, sizeof(char *), array_string);)

        free(toks);

        char **data = (char **)names.data;
        for (size_t i = 0; i < names.size; i++)
                free(data[i]);

        free_array(&names, sizeof(char *));

        return nullptr;
}

int main() 
{
        const char *str = "if {}{}{}канстанта если (канстанта ewew111>===0 | y!=2){const hel111lo3=-2.21e12; y = x * 32 ^ result; if (zero < 2 & keyval != 2.2212*321) -0.321e-1 - 1.22 Gar1k; while (result <= 5.0) 32;}";

        create_ast(str);
        return 0;
}

