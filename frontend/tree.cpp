#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <logs.h>
#include <limits.h>

#include <frontend/tree.h>

ast_node *set_ast_number(ast_node *n, double number)
{
        assert(n);
        assert(n->type == AST_NODE_NUMBER);

        if (n->type != AST_NODE_NUMBER)
                return nullptr;

        n->data.number = number;
        return n;
}

ast_node *set_ast_ident(ast_node *n, const char *ident)
{
        assert(n);
        assert(n->type == AST_NODE_IDENT);

        if (n->type != AST_NODE_IDENT)
                return nullptr;

        n->data.ident = ident;
        return n;
}

ast_node *set_ast_keyword(ast_node *n, int keyword)
{
        assert(n);
        assert(n->type == AST_NODE_KEYWORD);

        if (n->type != AST_NODE_KEYWORD)
                return nullptr;

        n->data.keyword = keyword;
        return n;
}

double ast_number(ast_node *n)
{
        assert(n);
        assert(n->type == AST_NODE_NUMBER);

        if (n->type == AST_NODE_NUMBER)
                return n->data.number;

        return 0;
}

const char *ast_ident(ast_node *n)
{
        assert(n);
        assert(n->type == AST_NODE_IDENT);

        if (n->type == AST_NODE_IDENT)
                return n->data.ident;

        return nullptr;
}

int ast_keyword(ast_node *n)
{
        assert(n);
        assert(n->type == AST_NODE_KEYWORD);

        if (n->type == AST_NODE_KEYWORD)
                return n->data.keyword;

        return 0;
}

ast_node *copy_tree(ast_node *n)
{
        assert(n);

        ast_node *newbie = create_ast_node(n->type);
        if (!newbie)
                return nullptr;

        newbie->type = n->type;
        newbie->data = n->data;

        if (n->left) {
                newbie->left  = copy_tree(n->left);
                if (!newbie->left) {
                        free_tree(newbie);
                        return nullptr;
                }
        }

        if (n->right) {
                newbie->right = copy_tree(n->right);
                if (!newbie->right) {
                        free_tree(newbie);
                        return nullptr;
                }
        }

        return newbie;
}


void free_tree(ast_node *root)
{
        assert(root);

        if (root->left)
                free_tree(root->left);
        if (root->right)
                free_tree(root->right);

        free(root);
}

ast_node *create_ast_keyword(int keyword) 
{
        ast_node *newbie = create_ast_node(AST_NODE_KEYWORD);
        if (!newbie)
                return nullptr;

        set_ast_keyword(newbie, keyword);
        return newbie;
}

ast_node *create_ast_number(int number) 
{
        ast_node *newbie = create_ast_node(AST_NODE_NUMBER);
        if (!newbie)
                return nullptr;

        set_ast_number(newbie, number);
        return newbie;
}

ast_node *create_ast_ident(const char *ident) 
{
        ast_node *newbie = create_ast_node(AST_NODE_IDENT);
        if (!newbie)
                return nullptr;

        set_ast_ident(newbie, ident);
        return newbie;
}

ast_node *create_ast_node(int type)
{
        assert(type == AST_NODE_IDENT   ||
               type == AST_NODE_NUMBER  ||
               type == AST_NODE_KEYWORD );

$       (ast_node *newbie = (ast_node *)calloc(1, sizeof(ast_node));)
        if (!newbie) {
                fprintf(logs, "Can't create ast_node\n");
                return nullptr;
        }

        newbie->type       = type;
        newbie->hash       = 0x00000;
        newbie->left       = nullptr;
        newbie->right      = nullptr;
        newbie->data.ident = nullptr;

        return newbie;
}

void visit_tree(ast_node *root, void (*action)(ast_node *nd))
{
        assert(root);
        assert(action);

        if (!root)
                return;

        action(root);

        if (root->left)  
                visit_tree(root->left, action);
        if (root->right) 
                visit_tree(root->right, action);
}

