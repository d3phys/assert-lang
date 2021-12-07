#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <logs.h>
#include <limits.h>

#include <frontend/tree.h>

double node_number(node *n)
{
        assert(n);
        assert(n->type == AST_NODE_NUMBER);

        if (n->type == AST_NODE_NUMBER)
                return n->data.number;

        return 0;
}

const char *node_ident(node *n)
{
        assert(n);
        assert(n->type == AST_NODE_IDENT);

        if (n->type == AST_NODE_IDENT)
                return n->data.ident;

        return nullptr;
}

int node_keyword(node *n)
{
        assert(n);
        assert(n->type == AST_NODE_KEYWORD);

        if (n->type == AST_NODE_KEYWORD)
                return n->data.keyword;

        return 0;
}

node *copy_tree(node *n)
{
        assert(n);

        node *newbie = create_node(n->type);
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


void free_tree(node *root)
{
        assert(root);

        if (root->left)
                free_tree(root->left);
        if (root->right)
                free_tree(root->right);

        free(root);
}

node *create_node(int type)
{
        assert(type == AST_NODE_IDENT   ||
               type == AST_NODE_NUMBER  ||
               type == AST_NODE_KEYWORD );

$       (node *newbie = (node *)calloc(1, sizeof(node));)
        if (!newbie) {
                fprintf(logs, "Can't create node\n");
                return nullptr;
        }

        newbie->type       = type;
        newbie->hash       = 0x00000;
        newbie->left       = nullptr;
        newbie->right      = nullptr;
        newbie->data.ident = nullptr;

        return newbie;
}

void visit_tree(node *root, void (*action)(node *nd))
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

