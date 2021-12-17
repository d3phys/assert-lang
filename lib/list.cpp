#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <logs.h>
#include <list.h>


static node *realloc_list(list *const lst, const size_t new_cap);
static inline int validate_position(list *const lst, ptrdiff_t pos);

static ptrdiff_t list_insert(list *const lst, 
                             item_t data, ptrdiff_t next, ptrdiff_t prev);

/*
 * Oooh my... Is it TXLib style?
 */
#define verify_list(lst) $(verify_list(lst))

#define log(fmt, ...) fprintf(logs, fmt, ##__VA_ARGS__);

/*
 * Exchanges the contents of the container with those of other.
 * Does not invoke any move, copy, or swap operations on individual elements.
 */
void swap_list(list *const lst1, list *const lst2)
{
        assert(lst1 && lst2);
        verify_list(lst1);
        verify_list(lst2);

        list temp  = *lst2;
        *lst1      = *lst2;
        *lst2      = temp;
}

/*
 * Gets physical address (index) from logical index.
 *      - O(n) complexity at random list
 *      - O(1) complexity after 'make_linear_list()'
 */
ptrdiff_t get_real_pos(list *const lst, ptrdiff_t log_pos)
{
        assert(lst);
        if (validate_position(lst, log_pos))
                return 0;

        verify_list(lst);

        if (lst->linear)
                return log_pos;

        ptrdiff_t pos = lst->head;
        while (--log_pos > 0)
                pos = lst->nodes[pos].next;

        return pos;
}

/*
 * Rearranges list. And makes logical index = physical index
 *     - O(n) complexity
 *     - O(1) additional memory required
 */
ptrdiff_t make_linear_list(list *const lst)
{
        assert(lst && lst->nodes);
        verify_list(lst);

        if (lst->linear)
                return lst->head;

        node temp = {0};

        node *nodes  = lst->nodes;
        ptrdiff_t nd = lst->head;

        ptrdiff_t wr = 0;
        for (wr = 1; nd != 0; wr++) {
                if (wr != nd) {
                        if (nodes[wr].prev != FREE_PREV) {
                                nodes[nodes[wr].prev].next = nd;
                                nodes[nodes[wr].next].prev = nd;
                        }

                        temp      = nodes[wr];
                        nodes[wr] = nodes[nd];
                        nodes[nd] = temp;
                }

                nodes[wr].prev = wr - 1;
                nodes[wr - 1].next = wr;

                nd = nodes[wr].next;
        }

        lst->free = wr;

        for (; wr < lst->capacity; wr++)
                nodes[wr].next = wr + 1;

        nodes[0] = {
                .next = 0,
                .prev = 0,
                .data = 0,
        };

        nodes[lst->capacity - 1].next = 0;

        lst->tail   = lst->n_nodes;
        lst->head   = 1;
        lst->linear = 1;

        verify_list(lst);
        return lst->head;
}

static ptrdiff_t list_insert(list *const lst, 
                             item_t data, ptrdiff_t next, ptrdiff_t prev)
{
        assert(lst && lst->nodes);
        verify_list(lst);

        if (!lst->free) {
$               (node *nodes = realloc_list(lst, lst->capacity * 2);)
                if (!nodes)
                        return 0;
        }

        ptrdiff_t free = lst->free;

        lst->free = lst->nodes[free].next;

        lst->nodes[free] = { 
                .next = next,
                .prev = prev,
                .data = data, 
        };

        if (!prev)
                lst->head = free;

        if (!next)
                lst->tail = free;

        lst->n_nodes++;
        return free;
}

list *construct_list(list *const lst, const size_t cap)
{
        assert(lst && !lst->nodes);
        assert(cap);

        lst->free     = 0;
        lst->head     = 0;
        lst->tail     = 0;
        lst->capacity = 1;
        lst->n_nodes  = 0;

$       (node *nodes = realloc_list(lst, cap + 1);)
        if (!nodes)
                return nullptr;

        lst->nodes[0] = { 
                .next = 0,
                .prev = 0,
                .data = 0, 
        };

        verify_list(lst);
        return lst;
}

void destruct_list(list *const lst)
{
        assert(lst && lst->nodes);

$       (free(lst->nodes);)

        lst->capacity =  0;
}

ptrdiff_t list_delete(list *const lst, ptrdiff_t pos)
{
        assert(lst && lst->nodes);
        verify_list(lst);

        assert(lst->nodes[pos].prev != FREE_PREV);
        if (lst->nodes[pos].prev == FREE_PREV) {
                log("Unable to release freed item\n");
                return pos;
        }

        node *const nodes = lst->nodes;

        if (lst->head == pos)
                lst->head = lst->nodes[pos].next;
        else
                nodes[nodes[pos].prev].next = nodes[pos].next; 

        if (lst->tail == pos)
                lst->tail = lst->nodes[pos].prev;
        else
                nodes[nodes[pos].next].prev = nodes[pos].prev; 

        nodes[pos].prev = FREE_PREV;
        nodes[pos].data = FREE_DATA;

        nodes[pos].next = lst->free;
        lst->free = pos;
        lst->n_nodes--;

        verify_list(lst);
        return pos;
}

ptrdiff_t list_insert_after(list *const lst, ptrdiff_t pos, item_t data)
{
        assert(lst && lst->nodes);
        if (validate_position(lst, pos))
                return 0;

        verify_list(lst);

        ptrdiff_t ins = list_insert(lst, data, lst->nodes[pos].next, pos);
        if (ins && pos) {
                if (lst->nodes[pos].next)
                        lst->nodes[lst->nodes[pos].next].prev = ins;

                lst->nodes[pos].next = ins;
        }

        verify_list(lst);
        return ins;
}

ptrdiff_t list_insert_before(list *const lst, ptrdiff_t pos, item_t data)
{
        assert(lst && lst->nodes);
        if (validate_position(lst, pos))
                return 0;

        verify_list(lst);

        ptrdiff_t ins = list_insert(lst, data, pos, lst->nodes[pos].prev);
        if (ins && pos) {
                if (lst->nodes[pos].prev)
                        lst->nodes[lst->nodes[pos].prev].next = ins;

                lst->nodes[pos].prev = ins;
        }

        verify_list(lst);
        return ins;
}

ptrdiff_t list_insert_front(list *const lst, item_t data)
{
        assert(lst && lst->nodes);
        verify_list(lst);

        return list_insert_before(lst, lst->head, data);
}

ptrdiff_t list_insert_back(list *const lst, item_t data)
{
        assert(lst && lst->nodes);
        verify_list(lst);

        return list_insert_after(lst, lst->tail, data);
}

/**
 * @brief Reallocates list memory
 *
 * @param stk List to reallocate
 * @param stk List's new capacity
 *
 * It is ANSI realloc() function wrapper.
 */
static node *realloc_list(list *const lst, const size_t new_cap)
{
        assert(lst);
        assert(new_cap > lst->capacity);

        size_t cap = new_cap * sizeof(node);

$       (node *const nodes = (node *)realloc(lst->nodes, cap);)
        if (!nodes)
                return nullptr;

        for (size_t n = lst->capacity; n < new_cap; n++)
                nodes[n] = { 
                        .next = n + 1,
                        .prev = FREE_PREV,
                        .data = FREE_DATA, 
                };

        nodes[new_cap - 1].next = lst->free; 

$       (lst->free = lst->capacity;)

        lst->capacity = new_cap;
        lst->nodes    = nodes;

        return nodes;
}

static inline int validate_position(list *const lst, ptrdiff_t pos) 
{
        int ret = pos < 0 || pos >= lst->capacity; 
        if (ret)
                log("<font color=\"red\">Invalid index</font>\n");

        return ret;
}

#undef verify_list

#ifdef DEBUG
ptrdiff_t verify_list(list *const lst)
{
        assert(lst);

        size_t n_nodes = 0;
        node *nodes = lst->nodes;
        ptrdiff_t it = 0;

        if (nodes[0].prev != 0 || nodes[0].next != 0)
                goto verify_error;

        it = lst->head;
        if (nodes[it].prev != 0)
                goto verify_error;

        for (it = 1; it < lst->capacity; it++) {

                if (nodes[it].prev == FREE_PREV) {
                        if (nodes[it].next < 0 || nodes[it].next > lst->capacity)
                                goto verify_error;

                        continue;
                }

                if (nodes[it].prev < 0 || nodes[it].prev > lst->capacity) 
                        goto verify_error;

                if (nodes[it].next < 0 || nodes[it].next > lst->capacity) 
                        goto verify_error;

                if (nodes[nodes[it].prev].next != it && it != lst->head)
                        goto verify_error;

                if (nodes[nodes[it].next].prev != it && it != lst->tail)
                        goto verify_error;

                n_nodes++;
        }
        
        it = lst->free;
        for (ptrdiff_t i = 1; i < lst->capacity; i++) {
                if (nodes[it].prev != FREE_PREV && it)
                        goto verify_error;

                if (nodes[it].next < 0 || nodes[it].next > lst->capacity)
                        goto verify_error;

                it = nodes[it].next;
        }

        if (it != 0)
                goto verify_error;

        it = lst->tail;
        if (nodes[it].next != 0)
                goto verify_error;

        if (lst->n_nodes != n_nodes) {
                it = -231;
                goto verify_error;
        }


        log("<font color=\"green\">verification passed</font>\n");
        return 0;

verify_error:
        log("<font color=\"red\">DAMAGED NODE:  %ld</font>\n", it);
$       (dump_list(lst);)
        destruct_list(lst);
        exit(1);
        return it;
}
#endif

void dump_list(const list *const lst)
{
        assert(lst);

        static unsigned dump_num = 0;

        static char buf[512] = {0};
        snprintf(buf, sizeof(buf), "log/list_dump%u.dot", dump_num);

        FILE *file = fopen(buf, "w");

        static const char HEADER[] = R"(
                                        digraph {
                                                dpi      = 57
                                                fontname = "Courier"

                                                edge [color = darkgrey, arrowhead = onormal, arrowsize = 1.6, penwidth = 1.2]

                                                graph[fillcolor = lightgreen, ranksep = 1.3, nodesep = 0.5,
                                                      style = "rounded, filled",color = green, penwidth = 2]

                                                node [penwidth = 2, shape = box, color = grey, 
                                                      fillcolor = white, style = "rounded, filled", fontname = "Courier"]

                                                compound  =  true;
                                                newrank   =  true;
                                                rankdir   =  LR; 
                                        )";

        fprintf(file, HEADER);

        fprintf(file, "0");
        for (size_t i = 1; i < lst->capacity; i++) {
                fprintf(file, "-> %lu", i);
        }
        fprintf(file, "[style=invis, weight=1, minlen=\"1.5\"]");


        fprintf(file, "free->%ld:n[color=cadetblue]\n", lst->free);
        fprintf(file, "tail->%ld:p[color=cadetblue]\n", lst->tail);
        fprintf(file, "head->%ld:n[color=cadetblue]\n", lst->head);


        for (size_t i = 0; i < lst->capacity; i++) {
                fprintf(file, "subgraph cluster%lu { \n"
                               "       label = %lu;  \n"
                               "       fontsize= 20; \n", i, i);


               fprintf(file, "       %lu [shape=record, label=\"<p>prev: %ld | data: %p | <n>next: %ld\"] \n} \n",
                       i, lst->nodes[i].prev, lst->nodes[i].data, lst->nodes[i].next);

                if (lst->nodes[i].prev != FREE_PREV)
                        fprintf(file, "%lu:n -> %ld:n[color=darkgoldenrod2, style=dashed]\n", i, lst->nodes[i].next);
                else
                        fprintf(file, "%lu:n -> %ld:n[color=mediumpurple4 ]\n", i, lst->nodes[i].next);

                if (lst->nodes[i].prev != FREE_PREV)
                        fprintf(file, "%lu:p -> %ld:p[color=darkslategray, style=dashed]\n", i, lst->nodes[i].prev);
        }

        fprintf(file, "\n}");
        fclose(file);

        snprintf(buf, sizeof(buf), "dot -Tsvg log/list_dump%u.dot -o log/list_dump%u.svg", dump_num, dump_num);
        system(buf);

        log("\n<img src=\"log/list_dump%u.svg\"/>\n", dump_num);

        dump_num++;
}

