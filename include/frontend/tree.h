#ifndef TREE_H
#define TREE_H

#include <stdint.h>

enum ast_node_type {
        AST_NODE_KEYWORD  = 0x01,
        AST_NODE_IDENT    = 0x02,
        AST_NODE_NUMBER   = 0x03,
};

struct ast_node {
        ast_node *left  = nullptr;
        ast_node *right = nullptr;

        uint32_t hash = 0;

        int type = 0;

        union {
                const char *ident;
                double     number;
                int       keyword;
        } data;
};

int         ast_keyword(ast_node *n);
double      ast_number (ast_node *n);
const char *ast_ident  (ast_node *n);

ast_node *set_ast_keyword(ast_node *n, int keyword);
ast_node *set_ast_number (ast_node *n, double number);
ast_node *set_ast_ident  (ast_node *n, const char *ident);

ast_node *create_ast_keyword(int keyword);
ast_node *create_ast_number(double number);
ast_node *create_ast_ident  (const char *ident);

/*
 * Jumps from node to node recursively. 
 * Then applies 'action' to the current node.
 *
 * Note! It calls action() before next jump.
 */
void visit_tree(ast_node *root, void (*action)(ast_node *nd));

void free_tree(ast_node *root);
ast_node *create_ast_node(int type);
ast_node *copy_tree(ast_node *n);

int save_ast_node(FILE *file, ast_node *const node);
void save_ast_tree(FILE *file, ast_node *const tree);

size_t calc_tree_size(ast_node *n);
ast_node *compare_trees(ast_node *t1, ast_node *t2);

#define TREE_DEBUG

#ifdef TREE_DEBUG
void dump_tree(ast_node *root);
#else /* TREE_DEBUG */
static inline void dump_tree(ast_node *root) {}
#endif /* TREE_DEBUG */

#endif /* TREE_H */


