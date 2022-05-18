#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <logs.h>

#include <ast/tree.h>
#include <ast/keyword.h>

#include <frontend/keyword.h>
#include <frontend/token.h>

//#define ERROR_TRACE

#define require(__keyword)                 \
do {                                       \
        if (keyword(*toks) != __keyword) { \
                return syntax_error(toks); \
        }                                  \
        move(toks);                        \
} while (0)

#ifdef ERROR_TRACE

#define syntax_error(toks)                                               \
        fprintf(logs, html(red, bold("Syntax error in %s, line: %d\n")), \
                      __PRETTY_FUNCTION__, __LINE__),                    \
        dump_tokens(*toks), nullptr

#define core_error(toks)                                                 \
        fprintf(logs, html(red, bold("Core error in %s, line: %d\n")),   \
                      __PRETTY_FUNCTION__, __LINE__),                    \
        dump_tokens(*toks), nullptr

#else 

static ast_node *syntax_error(token **toks);
static ast_node *core_error  (token **toks);
static void print_token(token *tok);

#endif /* ERROR_TRACE */

static token *next(token **toks);
static token *move(token **toks);

static int       keyword(token *tok);
static num_t     *number(token *tok);
static const char *ident(token *tok);

ast_node    *grammar_rule(token **toks);
ast_node     *assign_rule(token **toks);
ast_node     *define_rule(token **toks);
ast_node      *block_rule(token **toks);
ast_node  *statement_rule(token **toks);
ast_node *expression_rule(token **toks);
ast_node   *additive_rule(token **toks);
ast_node     *factor_rule(token **toks);
ast_node   *exponent_rule(token **toks);
ast_node    *boolean_rule(token **toks);
ast_node    *logical_rule(token **toks);

ast_node         *if_rule(token **toks);
ast_node      *while_rule(token **toks);
ast_node      *array_rule(token **toks);
ast_node      *ident_rule(token **toks);
ast_node     *number_rule(token **toks);
ast_node   *function_rule(token **toks);

ast_node *create_ast(const char *str);

ast_node *create_ast(const char *source_code)
{
        assert(source_code);

        array names = {0};
        token *toks = tokenize(source_code, &names);
        fprintf(logs, "\n\n%s\n\n", source_code);
$       (dump_tokens(toks);)
$       (dump_array(&names, sizeof(char *), array_string);)

        token *iter = toks;

        ast_node *tree = grammar_rule(&iter);
        fprintf(logs, "\n\n%s\n\n", source_code);
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

        ast_node *root = nullptr;

        while (keyword(*toks) == KW_DEFINE || 
               keyword(*toks) == KW_ASSERT) { 

                ast_node *stmt = create_ast_keyword(AST_STMT);
                if (!stmt)
                        return core_error(toks);

                switch (keyword(*toks)) {

                case KW_ASSERT:
                        require(KW_ASSERT);
                        require(KW_OPEN);

                        stmt->right = assign_rule(toks);
                        if (!stmt->right)
                                return syntax_error(toks);

                        require(KW_CLOSE);
                        require(KW_SEMICOL);
                        break;
                case KW_DEFINE:
                        move(toks);

                        stmt->right = define_rule(toks);
                        if (!stmt->right)
                                return syntax_error(toks);
                        break;
                default:
                        assert(0);
                        break;
                }

                stmt->left = root;
                root = stmt;
        }

        if (keyword(*toks) != KW_STOP)
                return syntax_error(toks);

        return root;
}

ast_node *assign_rule(token **toks)
{
        assert(toks);

        ast_node *root = create_ast_keyword(AST_ASSIGN);
        if (!root)
                return core_error(toks);

        ast_node *constant = nullptr;
        if (keyword(*toks) == KW_CONST) {
                require(KW_CONST);
                constant = create_ast_keyword(AST_CONST);
                if (!constant)
                        return core_error(toks);
        }

        if (keyword(next(toks)) == KW_QOPEN)
                root->left = array_rule(toks);
        else
                root->left = ident_rule(toks);

        if (!root->left)
                return syntax_error(toks);

        root->left->left = constant; 

        require(KW_ASSIGN);

        root->right = expression_rule(toks);
        if (!root->right)
                return syntax_error(toks);

        return root;
}

ast_node *define_rule(token **toks)
{
        assert(toks);
$$
        ast_node *root = create_ast_keyword(AST_DEFINE);
$$
        if (!root)
                return core_error(toks);

$$
        ast_node *func = create_ast_keyword(AST_FUNC);
$$
        if (!func)
                return core_error(toks);
$$

        func->left = ident_rule(toks);
$$
        if (!func->left)
                return syntax_error(toks);
$$

        root->left = func;
$$

        require(KW_OPEN);
$$

        if (ident(*toks)) {
$$
                func->right = create_ast_keyword(AST_PARAM);
$$
                if (!func->right)
                        return core_error(toks);
$$

                func->right->right = ident_rule(toks);
$$
                if (!func->right->right)
                        return syntax_error(toks);

$$
                while (keyword(*toks) == KW_COMMA) {
$$
                        require(KW_COMMA);
                        ast_node *param = create_ast_keyword(AST_PARAM);
                        if (!param)
                                return core_error(toks);
$$

                        param->right = ident_rule(toks);
                        if (!param->right)
                                return syntax_error(toks);
$$

                        param->left = func->right;
                        func->right = param;
                }
        }
$$

        require(KW_CLOSE);

$$
        root->right = block_rule(toks);
$$
        if (!root->right)
                return syntax_error(toks);

        return root;
}

ast_node *block_rule(token **toks)
{
        assert(toks);

        ast_node *root = nullptr;
        if (keyword(*toks) == KW_BEGIN) {
                move(toks);

                do {
                        ast_node *stmt = create_ast_keyword(AST_STMT);
                        if (!stmt) 
                                return core_error(toks);

                        stmt->right = statement_rule(toks);
                        if (!stmt->right) 
                                return syntax_error(toks);

                        if (ast_keyword(stmt->right) == AST_STMT) {
                                ast_node *last = stmt->right;
                                while (last->left)
                                        last = last->left;

                                last->left = root;
                                root = stmt->right;
                                free(stmt);
                                continue;
                        }

                        stmt->left = root;
                        root = stmt;

                } while (keyword(*toks) != KW_END);

                move(toks);

                return root;
        }

        root = create_ast_keyword(AST_STMT);
        if (!root) 
                return core_error(toks);

        root->right = statement_rule(toks);
        if (!root->right) 
                return syntax_error(toks);

        return root;
}

ast_node *if_rule(token **toks)
{
        assert(toks);

        require(KW_IF);

        ast_node *root = create_ast_keyword(AST_IF);
        if (!root) 
                return core_error(toks);

        ast_node *decision = create_ast_keyword(AST_DECISN);
        if (!root) 
                return core_error(toks);

        root->right = decision;

        require(KW_OPEN);

        root->left = expression_rule(toks);
        if (!root->left) 
                return syntax_error(toks);

        require(KW_CLOSE);

        decision->left = block_rule(toks);
        if (!decision->left) 
                return syntax_error(toks);

        if (keyword(*toks) == KW_ELSE) {
                move(toks);

                decision->right = block_rule(toks);
                if (!decision->right) 
                        return syntax_error(toks);
        }

        return root;
}

ast_node *while_rule(token **toks)
{
        assert(toks);

        require(KW_WHILE);
        require(KW_OPEN);

        ast_node *root = create_ast_keyword(AST_WHILE);
        if (!root) 
                return core_error(toks);

        root->left = expression_rule(toks);
        if (!root->left) 
                return syntax_error(toks);

        require(KW_CLOSE);

        root->right = block_rule(toks);
        if (!root->right) 
                return syntax_error(toks);

        return root;
}

ast_node *statement_rule(token **toks)
{
        assert(toks);

        ast_node *root = nullptr;

        if (keyword(*toks) == KW_IF) {
                root = if_rule(toks);
                if (!root)
                        return syntax_error(toks);

                return root;
        }

        if (keyword(*toks) == KW_WHILE) {
                root = while_rule(toks);
                if (!root)
                        return syntax_error(toks);

                return root;
        }

        if (keyword(*toks) == KW_RETURN) {
                require(KW_RETURN);

                root = create_ast_keyword(AST_RETURN);
                if (!root)
                        return syntax_error(toks);

                root->right = expression_rule(toks);
                if (!root->right)
                        return syntax_error(toks);

                require(KW_SEMICOL);
                return root;
        }

        int assert = false;
        if (keyword(*toks) == KW_ASSERT) {
                assert = true;
                require(KW_ASSERT);
                require(KW_OPEN);
        }

        if (keyword(*toks) == KW_RETURN) {
                require(KW_RETURN);

                root = create_ast_keyword(AST_RETURN);
                if (!root)
                        return syntax_error(toks);

                root->right = expression_rule(toks);
                if (!root->right)
                        return syntax_error(toks);

        } else if (keyword(*toks) == KW_OUT) {
                require(KW_OUT);

                root = create_ast_keyword(AST_OUT);
                if (!root)
                        return syntax_error(toks);

                require(KW_OPEN);
                root->right = expression_rule(toks);
                if (!root->right)
                        return syntax_error(toks);
                require(KW_CLOSE);

        }  else {
                if (ident(*toks) && keyword(next(toks)) == KW_OPEN) {
                        root = function_rule(toks);
                        if (!root)
                                return syntax_error(toks);
                } else {
                        root = assign_rule(toks);
                        if (!root)
                                return syntax_error(toks);
                }
        }

        if (assert) {
                require(KW_CLOSE);
                require(KW_SEMICOL);
                return root;
        }

        require(KW_SEMICOL);
        ast_node *stmt = root;
        root = create_ast_keyword(AST_WHILE);
        if (!root)
                return core_error(toks);

        root->left = create_ast_number(0);
        if (!root->left)
                return core_error(toks);

        root->right = create_ast_keyword(AST_STMT);
        if (!root)
                return core_error(toks);

        root->right->right = stmt;
        return root;
}

ast_node *boolean_rule(token **toks)
{
        assert(toks);

        ast_node *root = nullptr;

        root = additive_rule(toks);
        if (!root) 
                return syntax_error(toks);

        while (keyword(*toks) == KW_ADD ||
               keyword(*toks) == KW_SUB) {

                ast_node *op = create_ast_node(AST_NODE_KEYWORD);
                if (!root) 
                        return core_error(toks);

                switch (keyword(*toks)) {
                case KW_ADD: 
                        set_ast_keyword(op, AST_ADD); 
                        break;
                case KW_SUB: 
                        set_ast_keyword(op, AST_SUB); 
                        break;
                default: 
                        assert(0); 
                        return syntax_error(toks);
                }

                move(toks);

                op->right = additive_rule(toks);
                if (!op->right) 
                        return core_error(toks);

                op->left = root;
                root = op;
        }

        return root;
}

ast_node *logical_rule(token **toks)
{ 
        assert(toks);

        ast_node *root = create_ast_node(AST_NODE_KEYWORD);
        if (!root) 
                return core_error(toks);

        root->left = boolean_rule(toks); 
        if (!root->left) 
                return syntax_error(toks);

        switch (keyword(*toks)) {
                case KW_LOW:    
                        set_ast_keyword(root, AST_LOW);    
                        break;
                case KW_EQUAL:  
                        set_ast_keyword(root, AST_EQUAL);  
                        break;
                case KW_GREAT:  
                        set_ast_keyword(root, AST_GREAT);  
                        break;
                case KW_NEQUAL: 
                        set_ast_keyword(root, AST_NEQUAL); 
                        break;
                case KW_GEQUAL: 
                        set_ast_keyword(root, AST_GEQUAL); 
                        break;
                case KW_LEQUAL: 
                        set_ast_keyword(root, AST_LEQUAL); 
                        break;
                default: 
                        return root->left;
        }

        move(toks);

        root->right = boolean_rule(toks); 
        if (!root->right) 
                return syntax_error(toks);

        return root;
}

ast_node *expression_rule(token **toks)
{ 
        assert(toks);

        ast_node *root = logical_rule(toks);
        if (!root) 
                return syntax_error(toks);

        while (keyword(*toks) == KW_OR ||
               keyword(*toks) == KW_AND) {

                ast_node *op = create_ast_node(AST_NODE_KEYWORD);
                if (!root) 
                        return core_error(toks);

                switch (keyword(*toks)) {
                case KW_OR: 
                        set_ast_keyword(op, AST_OR); 
                        break;
                case KW_AND: 
                        set_ast_keyword(op, AST_AND); 
                        break;
                default: 
                        assert(0); 
                        break;
                }

                move(toks);

                op->right = logical_rule(toks);
                if (!op->right) 
                        return core_error(toks);

                op->left = root;
                root = op;
        }

        return root;
}

ast_node *additive_rule(token **toks)
{ 
        assert(toks);

        ast_node *root = factor_rule(toks);
        if (!root) 
                return syntax_error(toks);

        while (keyword(*toks) == KW_MUL ||
               keyword(*toks) == KW_DIV) {

                ast_node *op = create_ast_node(AST_NODE_KEYWORD);
                if (!op) 
                        return core_error(toks);

                switch (keyword(*toks)) {
                case KW_MUL: 
                        set_ast_keyword(op, AST_MUL); 
                        break;
                case KW_DIV: 
                        set_ast_keyword(op, AST_DIV); 
                        break;
                default: 
                        assert(0); 
                        break;
                }

                move(toks);

                op->right = factor_rule(toks);
                if (!op->right) 
                        return core_error(toks);

                op->left = root;
                root = op;
        }

        return root;
}

ast_node *factor_rule(token **toks)
{ 
        assert(toks);

        ast_node *root = exponent_rule(toks);
        if (!root) 
                return syntax_error(toks);

        if (keyword(*toks) == KW_POW) {

                require(KW_POW);

                ast_node *op = create_ast_keyword(AST_POW);
                if (!op) 
                        return core_error(toks);

                op->right = exponent_rule(toks);
                if (!op->right) 
                        return syntax_error(toks);

                op->left = root;
                root = op;
        }

        return root;
}


ast_node *function_rule(token **toks) 
{
        assert(toks);

        ast_node *root = create_ast_keyword(AST_CALL);
        if (!root) 
                return core_error(toks);

        root->left = ident_rule(toks);
        if (!root->left) 
                return syntax_error(toks);

        require(KW_OPEN);

        if (keyword(*toks) == KW_CLOSE) {
                require(KW_CLOSE);
                return root;
        }

        root->right = create_ast_keyword(AST_PARAM);
        if (!root->right)
                return core_error(toks);

        root->right->right = expression_rule(toks);
        if (!root->right->right)
                return syntax_error(toks);

        while (keyword(*toks) == KW_COMMA) {
                require(KW_COMMA);

                ast_node *param = create_ast_keyword(AST_PARAM);
                if (!param)
                        return core_error(toks);

                param->right = expression_rule(toks);
                if (!param->right) 
                        return syntax_error(toks);

                param->left = root->right;
                root->right = param;
        }

        require(KW_CLOSE);

        return root;
}

ast_node *array_rule(token **toks)
{
        assert(toks);

        if (!ident(*toks))
                return syntax_error(toks);

        ast_node *root = ident_rule(toks);
        if (!root) 
                return syntax_error(toks);

        require(KW_QOPEN);

        root->right = expression_rule(toks);
        if (!root->right)
                return syntax_error(toks);

        require(KW_QCLOSE);

        return root;
}

ast_node *exponent_rule(token **toks) 
{
        assert(toks);

        ast_node *root = nullptr;

        if (ident(*toks)) {

                if (keyword(next(toks)) == KW_OPEN) {
                        root = function_rule(toks);
                        if (!root)
                                return syntax_error(toks);

                        return root;
                }

                if (keyword(next(toks)) == KW_QOPEN) {
                        root = array_rule(toks);
                        if (!root)
                                return syntax_error(toks);

                        return root;
                }

                root = ident_rule(toks);
                if (!root) 
                        return syntax_error(toks);

                return root;

        } else if (number(*toks)) {

                root = number_rule(toks);
                if (!root) 
                        return syntax_error(toks);

                return root;
        } else if (keyword(*toks) == KW_OPEN) {

                require(KW_OPEN);

                root = expression_rule(toks);
                if (!root) 
                        return syntax_error(toks);

                require(KW_CLOSE);
                return root;
        }

        root = create_ast_node(AST_NODE_KEYWORD);
        if (!root)
                return core_error(toks);

        switch (keyword(*toks)) {
        case KW_NOT:
                set_ast_keyword(root, AST_NOT);
                move(toks);
                root->right = exponent_rule(toks);
                if (!root->right)
                        return syntax_error(toks);

                return root;
        case KW_ADD:
                move(toks);
                /* free root */
                root = exponent_rule(toks);
                if (!root)
                        return syntax_error(toks);
                return root;
        case KW_SUB:
                set_ast_keyword(root, AST_SUB);
                move(toks);
                root->left = create_ast_number(0);
                root->right = exponent_rule(toks);
                if (!root->right)
                        return syntax_error(toks);

                return root;
        case KW_IN:
                set_ast_keyword(root, AST_IN);
                move(toks);
                require(KW_OPEN);
                require(KW_CLOSE);
                return root;
        default:
                break;
        }

        require(KW_OPEN);

        root->right = expression_rule(toks);
        if (!root->right) 
                return syntax_error(toks);

        require(KW_CLOSE);

        return root;
}

ast_node *number_rule(token **toks)
{ 
        assert(toks);
        if (!number(*toks))
                return syntax_error(toks); 

        ast_node *root = create_ast_number(*number(*toks));
        if (!root) 
                return core_error(toks);

        move(toks);

        return root;
}

ast_node *ident_rule(token **toks)
{
        assert(toks);
        if (!ident(*toks))
                return syntax_error(toks); 

        ast_node *root = create_ast_ident(ident(*toks));
        if (!root) 
                return core_error(toks);

        move(toks);

        return root;
}

#ifndef ERROR_TRACE
static ast_node *syntax_error(token **toks)
{
        assert(toks);

        fprintf(stderr, ascii(RED, "Syntax error -- "));
        print_token(*toks);
        //dump_tokens(*toks)
        return nullptr;
}

static ast_node *core_error(token **toks)
{
        assert(toks);

        fprintf(stderr, ascii(RED, "Core error -- "));
        print_token(*toks);
        return nullptr;
}
#endif

static token *next(token **toks) 
{
        assert(toks);
        return (*toks) + 1;
}

static token *move(token **toks)
{
        assert(toks);
        (*toks)++;
        return (*toks);
}

static int keyword(token *tok)
{
        assert(tok);

        if (tok->type == TOKEN_KEYWORD)
                return tok->data.keyword;

        return 0;
}

static num_t *number(token *tok)
{
        assert(tok);

        if (tok->type == TOKEN_NUMBER)
                return &tok->data.number;

        return nullptr;
}

static const char *ident(token *tok)
{
        assert(tok);

        if (tok->type == TOKEN_IDENT)
                return tok->data.ident;

        return nullptr;
}

static void print_token(token *toks) 
{
        assert(toks);

        switch (toks->type) {
        case TOKEN_KEYWORD:
                fprintf(stderr, "keyword: %s [%d or '%c']\n", 
                                keyword_string(toks->data.keyword), 
                                toks->data.keyword, toks->data.keyword);
                break;
        case TOKEN_NUMBER:
                fprintf(stderr, ascii(BLUE, "number: %ld\n"), toks->data.number);
                break;
        case TOKEN_IDENT:
                fprintf(stderr, ascii(WHITE, "ident: %s [%p]\n"), toks->data.ident, 
                                                                  toks->data.ident);
                break;
        default:
                fprintf(stderr, "Can't provide info");
                break;
        }
}
