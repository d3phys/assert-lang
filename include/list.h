#ifndef LIST_H
#define LIST_H

#include <stddef.h>

typedef void * item_t;

const item_t    FREE_DATA =  nullptr; 
const ptrdiff_t FREE_PREV = -1; 

static const size_t INITIAL_CAP = 4;

struct node {
        ptrdiff_t next = -1;        
        ptrdiff_t prev = -1;        
        item_t    data =  0;        
};

struct list {
        node *nodes = nullptr;

        size_t capacity = 0;

        ptrdiff_t head = 0; 
        ptrdiff_t tail = 0; 
        ptrdiff_t free = 0;

        size_t n_nodes = 0;

        int linear     = 0;
};

ptrdiff_t get_real_pos(list *const lst, ptrdiff_t log_pos);

void swap_list(list *const lst1, list *const lst2);

list *construct_list(list *const lst, const size_t cap = INITIAL_CAP);
void   destruct_list(list *const lst);

ptrdiff_t make_linear_list(list *const lst);

ptrdiff_t list_delete(list *const lst, ptrdiff_t index);

ptrdiff_t list_insert_after (list *const lst, ptrdiff_t index, item_t item);
ptrdiff_t list_insert_before(list *const lst, ptrdiff_t index, item_t item);

ptrdiff_t list_insert_back (list *const lst, item_t item);
ptrdiff_t list_insert_front(list *const lst, item_t item);

#ifdef DEBUG 
ptrdiff_t verify_list(list *const lst);
#else /* DEBUG */
static inline ptrdiff_t verify_list(list *const lst) { return 0; }
#endif /* DEBUG */

#define DUMP
#ifdef DUMP
void dump_list(const list *const lst);
#else /* DEBUG */
static inline void dump_list(list *const lst) {};
#endif /* DEBUG */


#endif /* LIST_H */

