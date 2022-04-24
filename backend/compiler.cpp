#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <array.h>
#include <iommap.h>
#include <stack.h>
#include <assert.h>
#include <logs.h>

#include <ast/tree.h>
#include <ast/keyword.h>

#include <backend/scope_table.h>
#include <backend/backend.h>

static int       keyword(ast_node *node);
static double    *number(ast_node *node);
static const char *ident(ast_node *node);

static ast_node *success   (ast_node *root);
static ast_node *syntax_err(ast_node *root);

static ast_node *dump_code (ast_node *root);

#define require(node, _ast_type)                        \
        do {                                            \
        if (node && keyword(node) != _ast_type)         \
                return syntax_error(root);              \
        } while (0)

#define require_ident(node)                             \
        do {                                            \
        if (node && node->type != AST_NODE_IDENT)       \
                return syntax_error(root);              \
        } while (0)

#define require_number(node)                            \
        do {                                            \
        if (node && node->type != AST_NODE_NUMBER)      \
                return syntax_error(root);              \
        } while (0)

/*
static ast_node *compile_return(ast_node *root, symbol_table *table);
static ast_node *compile_define(ast_node *root, symbol_table *table);
static ast_node *compile_stmt  (ast_node *root, symbol_table *table);
static ast_node *compile_assign(ast_node *root, symbol_table *table);
static ast_node *compile_expr  (ast_node *root, symbol_table *table);
static ast_node *compile_if    (ast_node *root, symbol_table *table);
static ast_node *compile_while (ast_node *root, symbol_table *table);
static ast_node *compile_call  (ast_node *root, symbol_table *table);
*/



char *segment_memcpy(
        ac_segment *segment, 
        const void *data, 
        size_t size
) {
        assert(data);
        assert(segment);
        
        segment_alloc(segment, size);
        fprintf(stderr, "%p + %lu = %p\n", segment->data, segment->size, segment->data + segment->size);
        memcpy(segment->data + segment->size, data, size);

        char *ret = segment->data + segment->size;
        segment->size += size; 

        return ret;
}

char *segment_mmap(
        ac_segment *segment, 
        const void *data, 
        size_t size
) {
        assert(data);
        assert(segment);
        assert(!segment->data);
        
        if (!segment_alloc(segment, size))
                return nullptr;
                
        memcpy(segment->data, data, size);
        segment->size = size;
         
        return segment->data;
}

void segment_free(ac_segment *seg)
{
        assert(seg);
        seg->allocated = 0;
        free(seg->data);
}

void *segment_alloc(ac_segment *segment, size_t size) 
{ 
        assert(segment);
        assert(size);

        char *data       = segment->data;
        size_t allocated = segment->allocated;

        if (!allocated)
                allocated = SEG_ALLOC_INIT;   

        while (size + segment->size > allocated)
                allocated *= 2;

        if (allocated > segment->allocated) {
                data = (char *)realloc(segment->data, allocated);
                if (!data) {
                        int saved_errno = errno;

                        fprintf(stderr, "Can't allocate segment data: %s", strerror(errno));

                        errno = saved_errno;
                        return nullptr;
                }        
        }

        segment->allocated = allocated;
        segment->data      = data;
        
        return data;
}

static ast_node *dump_code(ast_node *root)
{
        assert(root);
        fprintf(stderr, ";");
        save_ast_tree(stderr, root);
        fprintf(stderr, "\n");
        return root;
}
