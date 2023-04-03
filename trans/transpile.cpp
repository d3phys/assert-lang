#include <stdio.h>
#include <stdlib.h>
#include <iommap.h>
#include <logs.h>
#include <assert.h>
#include <ast/tree.h>
#include <ast/keyword.h>
#include <frontend/keyword.h>
#include <trans/transpile.h>

static int INDENT = 0;
static int INDENT_SPACES = 8;

static void indent()
{
        INDENT += INDENT_SPACES;
}

static void unindent()
{
        INDENT -= INDENT_SPACES;
}

#define write_ind(fmt, ...)                        \
        do {                                       \
                fprintf(file, "%*s", INDENT, "");  \
        } while (0)

#define write(fmt, ...)                            \
        do {                                       \
                fprintf(file, fmt, ##__VA_ARGS__); \
        } while (0)

#define require(node, kw)     if (!node || keyword(node) != kw) { return trans_error(root); }
#define require_ident(node)   if (!node || !ident(node))        { return trans_error(root); }
#define require_keyword(node)   if (!node || !keyword(node))    { return trans_error(root); }
#define require_number(node)  if (!node || !number(node))       { return trans_error(root); }

static int       keyword(ast_node *root);
static num_t     *number(ast_node *root);
static const char *ident(ast_node *root);

static ast_node *trans_define(FILE *file, ast_node *root);
static ast_node *trans_while(FILE *file, ast_node *root);
static ast_node *trans_if(FILE *file, ast_node *root);
static ast_node *trans_define_param(FILE *file, ast_node *root);
static ast_node *trans_call_param(FILE *file, ast_node *root);
static ast_node *trans_call(FILE *file, ast_node *root);
static ast_node *trans_variable(FILE *file, ast_node *root);
static ast_node *trans_expr(FILE *file, ast_node *root);
static ast_node *trans_show(FILE *file, ast_node *root);
static ast_node *trans_out(FILE *file, ast_node *root);

static ast_node *success(ast_node *root)
{
        return nullptr;
}

static ast_node *trans_error(ast_node *root)
{
        assert(root);
        fprintf(stderr, ascii(RED, "Syntax error:\n"));
        save_ast_tree(stderr, root);
        $(dump_tree(root);)
        fprintf(stderr, "\n");
        return root;
}

static ast_node *transpile(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);

        if (root->left)
                transpile(file, root->left);

        write("trans\n");

        if (root->right)
                transpile(file, root->right);

        return success(root);
}

static ast_node *trans_out(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_OUT);
        write("%s", keyword_string(KW_OUT));
        write("%s", keyword_string(KW_OPEN));

        if (!root->right || root->left)
                return trans_error(root);

        error = trans_expr(file, root->right);
        if (error)
                return error;

        write("%s", keyword_string(KW_CLOSE));
        return success(root);
}

static ast_node *trans_variable(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        if (root->left) {
                require(root->left, AST_CONST);
                write("%s ", keyword_string(KW_CONST));
        }

        require_ident(root);
        write("%s", ident(root));

        if (root->right) {
                write("%s", keyword_string(KW_QOPEN));

                error = trans_expr(file, root->right);
                if (error)
                        return error;

                write("%s", keyword_string(KW_QCLOSE));
        }

        return success(root);
}

static ast_node *trans_call_param(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_PARAM);
        if (root->left) {
                error = trans_call_param(file, root->left);
                if (error)
                        return error;
                write("%s ", keyword_string(KW_COMMA));
        }

        if (!root->right)
                return trans_error(root);

        error = trans_expr(file, root->right);
        if (error)
                return error;

        return success(root);
}

static ast_node *trans_call(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_CALL);

        require_ident(root->left);
        write("%s", ident(root->left));
        write("%s", keyword_string(KW_OPEN));

        if (root->right) {
                require(root->right, AST_PARAM);
                error = trans_call_param(file, root->right); 
                if (error)
                        return error;
        }

        write("%s", keyword_string(KW_CLOSE));
        return success(root);
}

static ast_node *trans_assign(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_ASSIGN);

        require_ident(root->left);
        error = trans_variable(file, root->left);
        if (error)
                return error;

        write(" %s ", keyword_string(KW_ASSIGN));

        if (!root->right)
                return error;

        error = trans_expr(file, root->right);
        if (error)
                return error;

        return success(root);
}

static ast_node *trans_if(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_IF);
        write("%s ", keyword_string(KW_IF));
        write("%s",  keyword_string(KW_OPEN));

        if (!root->right)
                return trans_error(root);

        error = trans_expr(file, root->left);
        if (error)
                return error;

        write("%s ",  keyword_string(KW_CLOSE));

        ast_node *decision = root->right;
        require(decision, AST_DECISN);

        require(decision->left, AST_STMT);
        write("%s\n", keyword_string(KW_BEGIN));

        indent();

        require(decision->left, AST_STMT);
        error = trans_stmt(file, decision->left);
        if (error)
                return error;

        unindent();
        write_ind();
        write("%s", keyword_string(KW_END));

        if (decision->right) { 
                write(" %s ",  keyword_string(KW_ELSE));
                write("%s\n", keyword_string(KW_BEGIN));
                indent();

                require(decision->right, AST_STMT);
                error = trans_stmt(file, decision->right);
                if (error)
                        return error;

                unindent();
                write_ind();
                write("%s", keyword_string(KW_END));
        }

        write("\n\n");
        return success(root);
}

static ast_node *trans_while(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_WHILE);
        write("%s ", keyword_string(KW_WHILE));
        write("%s",  keyword_string(KW_OPEN));

        if (!root->right)
                return trans_error(root);

        error = trans_expr(file, root->left);
        if (error)
                return error;

        write("%s ", keyword_string(KW_CLOSE));
        write("%s\n", keyword_string(KW_BEGIN));

        indent();

        require(root->right, AST_STMT);
        error = trans_stmt(file, root->right);
        if (error)
                return error;

        unindent();
        write("%s\n", keyword_string(KW_END));

        return success(root);
}

static ast_node *trans_return(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_RETURN);
        write("%s ", keyword_string(KW_RETURN));

        if (!root->right)
                return trans_error(root);

        error = trans_expr(file, root->right);
        if (error)
                return error;

        write("%s\n", keyword_string(KW_SEMICOL));
        return success(root);
}

ast_node *trans_stmt(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);

        ast_node *error = nullptr;
        if (keyword(root->left) == AST_STMT) {
                error = trans_stmt(file, root->left);
                if (error)
                        return error;
        }

        write_ind();
        switch (keyword(root->right)) {
        case AST_DEFINE:
                return trans_define(file, root->right);
        case AST_RETURN:
                return trans_return(file, root->right);
        case AST_WHILE:
                return trans_while(file, root->right);
        case AST_IF:
                return trans_if(file, root->right);
        default:
                break;
        }

        write("%s", keyword_string(KW_ASSERT)); 
        write("%s", keyword_string(KW_OPEN)); 

        switch (keyword(root->right)) {
        case AST_ASSIGN:
                error = trans_assign(file, root->right);
                break;
        case AST_CALL:
                error = trans_call(file, root->right);
                break;
        case AST_OUT:
                error = trans_out(file, root->right);
                break;
        default:
                return trans_error(root);
        }

        if (error)
                return error;

        write("%s", keyword_string(KW_CLOSE)); 
        write("%s\n", keyword_string(KW_SEMICOL)); 

        return success(root);
}

static ast_node *trans_define_param(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        require(root, AST_PARAM);
        if (root->left) {
                error = trans_define_param(file, root->left);
                if (error)
                        return error;
                write("%s ", keyword_string(KW_COMMA));
        }

        require_ident(root->right);
        write("%s", ident(root->right));
        return success(root);
}

static ast_node *trans_define(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        if (keyword(root) != AST_DEFINE)
                return trans_error(root);

        write("%s ", keyword_string(KW_DEFINE));

        require(root->left, AST_FUNC);
        ast_node *func = root->left;

        require_ident(func->left);

        write("%s", ident(func->left));
        write("%s", keyword_string(KW_OPEN));

        if (func->right) {
                error = trans_define_param(file, func->right);
                if (error)
                        return error;
        }

        write("%s\n", keyword_string(KW_CLOSE));
        write("%s\n", keyword_string(KW_BEGIN));
        indent();

        error = trans_stmt(file, root->right);
        if (error)
                return error;

        unindent();
        write("%s\n\n", keyword_string(KW_END));

        return success(root);
}

static ast_node *trans_expr(FILE *file, ast_node *root)
{
        assert(file);
        assert(root);
        ast_node *error = nullptr;

        if (keyword(root) == AST_CALL)
                return trans_call(file, root);

        if (ident(root))
                return trans_variable(file, root);

        if (number(root)) {
                write("%lg", *number(root));
                return success(root);
        }

        if (keyword(root) == AST_IN) {
                write("%s", keyword_string(KW_IN));
                write("%s", keyword_string(KW_OPEN));

                if (root->left || root->right)
                        return trans_error(root);

                write("%s", keyword_string(KW_CLOSE));
                return success(root);
        }

        write("%s", keyword_string(KW_OPEN));

        if (root->left) {
                error = trans_expr(file, root->left);
                if (error)
                        return error;
        }

#define TRANS(AST, KW)                                 \
        case AST:                                      \
                write(" %s ", keyword_string(KW)); \
                break;

        switch (keyword(root)) {
                TRANS(AST_OR, KW_OR);
                TRANS(AST_ADD, KW_ADD);
                TRANS(AST_SUB, KW_SUB);
                TRANS(AST_MUL, KW_MUL);
                TRANS(AST_DIV, KW_DIV);
                TRANS(AST_POW, KW_POW);
                TRANS(AST_LOW, KW_LOW);
                TRANS(AST_NOT, KW_NOT);
                TRANS(AST_AND, KW_AND);
                TRANS(AST_GREAT,  KW_GREAT);
                TRANS(AST_EQUAL,  KW_EQUAL);
                TRANS(AST_NEQUAL, KW_NEQUAL);
                TRANS(AST_GEQUAL, KW_GEQUAL);
                TRANS(AST_LEQUAL, KW_LEQUAL);
                default:
                        return trans_error(root);
        }

        if (root->right) {
                error = trans_expr(file, root->right);
                if (error)
                        return error;
        }

        write("%s", keyword_string(KW_CLOSE));
        return success(root);
}

static int keyword(ast_node *root)
{
        if (!root)
                return 0;

        if (root->type == AST_NODE_KEYWORD)
                return ast_keyword(root);

        return 0;
}

static num_t* number(ast_node *root)
{
        assert(root);

        if (root->type == AST_NODE_NUMBER)
                return &root->data.number;

        return nullptr;
}

static const char *ident(ast_node *root)
{
        assert(root);

        if (root->type == AST_NODE_IDENT)
                return root->data.ident;

        return nullptr;
}

