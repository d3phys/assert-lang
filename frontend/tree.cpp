#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <logs.h>
#include <limits.h>

#include <frontend/tree.h>

/*
node *compare_trees(node *t1, node *t2)
{
        assert(t1);
        assert(t2);

        if (t1->type != t2->type)
                return t1;

        switch (t1->type) {
        case NODE_NUM:
                if (*node_num(t1) != *node_num(t2))
                        return t1;
                break;
        case NODE_VAR:
                if (*node_var(t1) != *node_var(t2))
                        return t1;
                break;
        case NODE_OP:
                if (*node_op(t1) != *node_op(t2))
                        return t1;
                break;
        dafault:
                assert(0);
                return nullptr;
        }

        if (((!t1->left  && t2->left)  || (t1->left && !t2->left)) ||
            ((!t1->right && t2->right) || (t1->right && !t2->right)))
               return t1; 

        node *t = nullptr;

        if (t1->left)
                t = compare_trees(t1->left, t2->left);
        if (t)
                return t;

        if (t1->right)
                t = compare_trees(t1->right, t2->right);
        if (t)
                return t;

        return nullptr;
}
*/
size_t calc_tree_size(node *n) 
{
        assert(n);

        if (n->left && n->right)
                return calc_tree_size(n->left) + 
                       calc_tree_size(n->right);
        if (n->right)
                return calc_tree_size(n->right);
        if (n->left)
                return calc_tree_size(n->left);

        return 1;
}

double *node_num(node *n)
{
        assert(n);
        assert(n->type == NUMBER);

        if (n->type == NUMBER)
                return &n->data.number;

        return 0;
}

const char *node_var(node *n)
{
        assert(n);
        assert(n->type == VARIABLE);

        if (n->type == VARIABLE)
                return n->data.variable;

        return nullptr;
}

/*
unsigned *node_op(node *n)
{
        assert(n);
        assert(n->type == NODE_OP);

        if (n->type == NODE_OP)
                return &n->data.op;

        return 0;
}
*/

node *copy_tree(node *n)
{
        assert(n);

        node *newbie = create_node();
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

node *create_node()
{
$       (node *newbie = (node *)calloc(1, sizeof(node));)
        if (!newbie) {
                fprintf(logs, "Can't create node\n");
                return newbie;
        }

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

