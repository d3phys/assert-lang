#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <logs.h>
#include <frontend/grammar.h>
#include <frontend/tree.h>
#include <frontend/token.h>

#define ERROR_TRACE


#ifdef ERROR_TRACE
#define syntax_error(toks) fprintf(logs, html(red, bold("Syntax error in %s, line: %d\n")), __PRETTY_FUNCTION__, __LINE__), nullptr
#define core_error(toks)   fprintf(logs, html(red, bold("Core error in %s, line: %d\n"  )), __PRETTY_FUNCTION__, __LINE__), nullptr
#else
static ast_node *syntax_error(token **toks);
static ast_node *core_error  (token **toks);
#endif

static token *next(token **toks);
static token *move(token **toks);

static int       keyword(token **tok);
static double    *number(token **tok);
static const char *ident(token **tok);

ast_node     *factor_rule(token **toks);
ast_node      *block_rule(token **toks);
ast_node      *ident_rule(token **toks);
ast_node     *number_rule(token **toks);
ast_node    *grammar_rule(token **toks);
ast_node   *additive_rule(token **toks);
ast_node  *condition_rule(token **toks);
ast_node  *statement_rule(token **toks);
ast_node *expression_rule(token **toks);

ast_node *create_ast(const char *str);

int main() 
{
        const char *str = "{ if (x == 3){ result = 1; }  result = 1 / (2 - 3) - 4; result = 2 * 32; x = 2; }";
        create_ast(str);
        return 0;
}

ast_node *create_ast(const char *str)
{
        assert(str);

        array names = {0};
        token *toks = tokenize(str, &names);
$       (dump_tokens(toks);)
$       (dump_array(&names, sizeof(char *), array_string);)

        token *iter = toks;

        ast_node *tree = grammar_rule(&iter);
        $(dump_tree(tree);)

        free_tree(tree);
        free(toks);

        char **data = (char **)names.data;
        for (size_t i = 0; i < names.size; i++) {
                free(data[i]);
        }

        free_array(&names, sizeof(char *));

        return nullptr;
}

ast_node *grammar_rule(token **toks)
{
        assert(toks);

        ast_node *n = block_rule(toks);
        if (!n) return syntax_error(toks);

        $(dump_tokens(*toks);)
        if (keyword(toks) != KW_STOP)
                return syntax_error(toks);

        return n;
}

ast_node *block_rule(token **toks)
{
        assert(toks);

        ast_node *root = create_ast_node(AST_NODE_KEYWORD);
        if (!root) return core_error(toks);
        set_ast_keyword(root, AST_STMT);

        if (keyword(toks) == KW_BEGIN) {
                move(toks);

                root->right = statement_rule(toks);
                if (!root->right) return syntax_error(toks);

                while (keyword(toks) != KW_END) {
                        dump_tokens(*toks);

                        ast_node *stmt = create_ast_node(AST_NODE_KEYWORD);
                        if (!stmt) return core_error(toks);

                        set_ast_keyword(stmt, AST_STMT);

                        stmt->right = statement_rule(toks);
                        if (!stmt->right) return syntax_error(toks);

                        stmt->left = root;
                        root = stmt;
                        $(dump_tree(root);)
                }

                move(toks);

                return root;
        }

        root->right = statement_rule(toks);
        if (!root->right) return syntax_error(toks);

        return root;
}

ast_node *statement_rule(token **toks)
{
        assert(toks);

        if (keyword(toks) == KW_IF) {

                ast_node *n = create_ast_node(AST_NODE_KEYWORD);
                if (!n) return core_error(toks);
                set_ast_keyword(n, AST_IF);

                move(toks);

                if (keyword(toks) != KW_OPEN) return syntax_error(toks);

                move(toks);

                n->right = condition_rule(toks);
                if (!n->right) return syntax_error(toks);

                if (keyword(toks) != KW_CLOSE) return syntax_error(toks);

                move(toks);

                n->left = block_rule(toks);
                if (!n->left) return syntax_error(toks);

                return n;
        }

        if (ident(toks)) {

                ast_node *v = create_ast_node(AST_NODE_IDENT);
                if (!v) return core_error(toks);
                set_ast_ident(v, ident(toks));
                move(toks);
$$
                if (keyword(toks) != KW_ASSIGN) return syntax_error(toks);

$$
                ast_node *ass = create_ast_node(AST_NODE_KEYWORD);
                if (!ass) return core_error(toks);
                set_ast_keyword(ass, AST_ASSIGN);
                move(toks);
$$
                ass->left = v;
$$                
                ass->right = expression_rule(toks);
                if (!ass->right) return syntax_error(toks);

                if (keyword(toks) != KW_SEP) return syntax_error(toks);
                move(toks);
$$                
                dump_tokens(*toks);


                dump_tree(ass);
                return ass;

        }

        return nullptr;
}

ast_node *condition_rule(token **toks)
{
        assert(toks);

        ast_node *root = create_ast_node(AST_NODE_KEYWORD);
        if (!root) return core_error(toks);

        root->left = expression_rule(toks); 
        if (!root->left) return syntax_error(toks);

        switch (keyword(toks)) {
                case KW_LOW:    set_ast_keyword(root, AST_LOW);    break;
                case KW_EQUAL:  set_ast_keyword(root, AST_EQUAL);  break;
                case KW_GREAT:  set_ast_keyword(root, AST_GREAT);  break;
                case KW_NEQUAL: set_ast_keyword(root, AST_NEQUAL); break;
                case KW_GEQUAL: set_ast_keyword(root, AST_GEQUAL); break;
                case KW_LEQUAL: set_ast_keyword(root, AST_LEQUAL); break;

                default: return syntax_error(toks);
        }

        move(toks);

        root->right = expression_rule(toks); 
        if (!root->right) return syntax_error(toks);

        return root;
}


ast_node *expression_rule(token **toks)
{
        assert(toks);

        ast_node *root = nullptr;

        if (keyword(toks) == KW_ADD || 
            keyword(toks) == KW_SUB) 
        {
                ast_node *op = create_ast_node(AST_NODE_KEYWORD);
                if (!root) return core_error(toks);

                switch (keyword(toks)) {
                case KW_ADD: set_ast_keyword(op, AST_ADD); break;
                case KW_SUB: set_ast_keyword(op, AST_SUB); break;
                }

                move(toks);

                root->left = create_ast_node(AST_NODE_NUMBER);
                if (!root->left) return core_error(toks);
                set_ast_number(root->left, 0);

                root->right = additive_rule(toks);
                if (!root->right) return syntax_error(toks);

        } else {
                root = additive_rule(toks);
                if (!root) return syntax_error(toks);
        }

        while (keyword(toks) == KW_ADD ||
               keyword(toks) == KW_SUB) {

                ast_node *op = create_ast_node(AST_NODE_KEYWORD);
                if (!root) return core_error(toks);

                switch (keyword(toks)) {
                case KW_ADD: set_ast_keyword(op, AST_ADD); break;
                case KW_SUB: set_ast_keyword(op, AST_SUB); break;
                default: assert(0); break;
                }

                move(toks);

                op->right = additive_rule(toks);
                if (!op->right) return core_error(toks);

                op->left = root;
                root = op;
        }

        return root;
}

ast_node *additive_rule(token **toks)
{ 
        assert(toks);

        ast_node *root = factor_rule(toks);
        if (!root) return syntax_error(toks);

        while (keyword(toks) == KW_MUL ||
               keyword(toks) == KW_DIV) {

                ast_node *op = create_ast_node(AST_NODE_KEYWORD);
                if (!root) return core_error(toks);

                switch (keyword(toks)) {
                case KW_MUL: set_ast_keyword(op, AST_MUL); break;
                case KW_DIV: set_ast_keyword(op, AST_DIV); break;
                default: assert(0); break;
                }

                move(toks);

                op->right = factor_rule(toks);
                if (!op->right) return core_error(toks);

                op->left = root;
                root = op;
                $(dump_tree(root);)
        }

        return root;
}

ast_node *factor_rule(token **toks) 
{
        assert(toks);

        ast_node *root = nullptr;

        if (ident(toks)) {

                root = ident_rule(toks);
                if (!root) return syntax_error(toks);

        } else if (number(toks)) {

                root = number_rule(toks);
                if (!root) return syntax_error(toks);

        } else if (keyword(toks) == KW_OPEN) {
                move(toks);

                root = expression_rule(toks);
                if (!root) return syntax_error(toks);

                if (keyword(toks) != KW_CLOSE) return syntax_error(toks);
                move(toks);
        }

        return root;
}

ast_node *number_rule(token **toks)
{ 
        assert(toks);

        ast_node *n = create_ast_node(AST_NODE_NUMBER);
        if (!n) return core_error(toks);

        set_ast_number(n, *number(toks));
        move(toks);

        return n;
}

ast_node *ident_rule(token **toks)
{
        assert(toks);
        ast_node *n = create_ast_node(AST_NODE_IDENT);
        if (!n) return core_error(toks);

        set_ast_ident(n, ident(toks));
        move(toks);

        return n;
}

#ifndef ERROR_TRACE
static ast_node *syntax_error(token **toks)
{
        assert(toks);

        printf("Syntax error\n");
        return nullptr;
}

static ast_node *core_error(token **toks)
{
        assert(toks);

        printf("Core error: \n");
        return nullptr;
}
#endif

static token *next(token **toks) 
{
        assert(toks);
        return *(toks + 1);
}

static token *move(token **toks)
{
        assert(toks);
        (*toks)++;
        return (*toks);
}

static int keyword(token **tok)
{
        assert(tok && *tok);

        if ((*tok)->type == TOKEN_KEYWORD)
                return (*tok)->data.keyword;

        return 0;
}

static double *number(token **tok)
{
        assert(tok && *tok);

        if ((*tok)->type == TOKEN_NUMBER)
                return &(*tok)->data.number;

        return nullptr;
}

static const char *ident(token **tok)
{
        assert(tok && *tok);

        if ((*tok)->type == TOKEN_IDENT)
                return (*tok)->data.ident;

        return nullptr;
}
