/**
 * @file
 * @brief  Stack
 * @author d3phys
 * @date   07.10.2021
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <logs.h>

#include <stack.h>


#define UNPROTECT
#ifdef UNPROTECT
#undef HASH_PROTECT
#undef CANARY_PROTECT
#endif /* UNPROTECT */

static const int FILL_BYTE = 'u';
static inline item_t get_poison(const int byte);
static item_t POISON = get_poison(FILL_BYTE);

static const size_t INIT_CAP       = 8;
static const size_t CAP_FACTOR     = 2;
static const size_t CAP_MAX        = ~(SIZE_MAX >> 1);

static inline int expandable(const stack *const stk);
static inline int shrinkable(const stack *const stk);

#ifdef CANARY_PROTECT
static inline canary_t *left_canary (void *const items);
static inline canary_t *right_canary(void *const items, const size_t capacity);
#endif /* CANARY_PROTECT */

#ifdef HASH_PROTECT
static hash_t hash_stack(stack *const stk, uint32_t seed = SEED);
#endif /* HASH_PROTECT */

static inline const char *indicate_err(int condition);
static inline void set_error(int *const error, int value);

static item_t *realloc_stack(const stack *const stk, const size_t capacity);

static int verify_stack(stack *const stk);
static int verify_empty_stack(const stack *const stk);

static item_t item_stack(stack *const stk, size_t index, int *const error);


/**
 * @brief Calculates stk hash
 *
 * @param stk  Stack
 * @param seed Hash algorithm seed
 *
 * Calculates hash using murmur2 hash algorithm. 
 * Uses stk's location in memory and check every byte. 
 */
#ifdef HASH_PROTECT
static hash_t hash_stack(stack *const stk, uint32_t seed)
{
        assert(stk);

        hash_t hash = stk->hash;
        stk->hash = 0;

        hash_t  stk_hash = murmur_hash(stk, sizeof(stack), seed);
        hash_t data_hash = murmur_hash(stk->items, stk->capacity * sizeof(item_t), seed);

        stk->hash = hash;
        return stk_hash ^ data_hash;
}
#endif /* HASH_PROTECT */

/**
 * @brief Reallocates stk memory
 *
 * @param stk Stack to reallocate
 * @param stk Stack's new capacity
 *
 * It is ANSI realloc() function wrapper.
 * It allocates additional memory and repositions canaries.
 * if canary protection defined.
 */
static item_t *realloc_stack(const stack *const stk, const size_t capacity)
{
        assert(stk);
        size_t cap = capacity * sizeof(item_t);
        item_t *items = stk->items;

#ifdef CANARY_PROTECT
        size_t can_cap = cap + sizeof(void *) - cap % sizeof(void *) + 2 * sizeof(canary_t);

        if (items)
                items = (item_t *)left_canary(stk->items);

$       (items  = (item_t *)realloc((void *)items, can_cap);)
$       (items  = (item_t *)((char *)items + sizeof(canary_t));)
#else
$       (items  = (item_t *)realloc((void *)items, cap);)
#endif /* CANARY_PROTECT */

        if (!items) {
                fprintf(logs, "Invalid stk reallocation: %s\n", strerror(errno));
                return nullptr;
        }

$       (memset(items + stk->size, FILL_BYTE, cap - stk->size * sizeof(item_t));)

#ifdef CANARY_PROTECT
        *right_canary(items, capacity) = CANARY ^ (size_t)items;
        *left_canary (items)           = CANARY ^ (size_t)items;
#endif

        return items;
}

stack *construct_stack(stack *const stk, int *const error)
{
        assert(stk);
        item_t *items = nullptr;
        int err = 0;

#ifndef UNPROTECT
$       (err = verify_empty_stk(stk);)
#endif  /* UNPROTECT */

        if (err) {
                fprintf(logs, "Can't construct (stk is not empty)\n");
                goto finally;
        }

        items = realloc_stack(stk, INIT_CAP);
        if (!items) {
                fprintf(logs, "Invalid stk memory allocation\n");
                err = STK_BAD_ALLOC;
                goto finally;
        }

        stk->capacity = INIT_CAP;
        stk->items    = items;
        stk->size     = 0;

#ifdef CANARY_PROTECT
        stk->left_canary  = CANARY;
        stk->right_canary = CANARY;
#endif /* CANARY_PROTECT */

#ifdef HASH_PROTECT
$       (stk->hash = hash_stk(stk);)
#endif /* HASH_PROTECT */

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

finally:
        if (err) {
                set_error(error, err);
                log_dump(stk);
                return nullptr;
        }

        return stk;
}

item_t *data_stack(stack *const stk, int *const error)
{
        assert(stk);
        int err = 0;

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

        if (err) {
                fprintf(logs, "Can't get data from invalid stack\n");
                goto finally;
        }

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

finally:
        if (err) {
                set_error(error, err);
                log_dump(stk);
        }

        return stk->items;
}

item_t top_stack(stack *const stk, int *const error)
{
        return item_stack(stk, stk->size - 1, error);
}

item_t bottom_stack(stack *const stk, int *const error)
{
        return item_stack(stk, 0, error);
}
        
static item_t item_stack(stack *const stk, size_t index, int *const error)
{
        assert(stk);
        int err = 0;

        item_t item = POISON;

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

        if (err) {
                fprintf(logs, "Can't pop item from invalid stk\n");
                goto finally;
        }

        if (stk->size == 0) {
                fprintf(logs, "Can't get item %lu from empty stack\n", index);
                err = STK_EMPTY_POP;
                goto finally;
        }

        if (stk->size - 1 < index) {
                fprintf(logs, "Can't get item %lu. Index %lu is too big\n", index, index);
                err = STK_EMPTY_POP;
                goto finally;                
        }

        item = stk->items[index];

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

finally:
        if (err) {
                set_error(error, err);
                log_dump(stk);
        }

        return item;        
}

item_t check_stack(stack *const stk, int *const error)
{
        assert(stk);
        int err = 0;

        item_t item = POISON;

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

        if (err) {
                fprintf(logs, "Can't pop item from invalid stk\n");
                goto finally;
        }

        if (stk->size == 0) {
                fprintf(logs, "Can't check an empty stk\n");
                err = STK_EMPTY_POP;
                goto finally;
        }

        item = stk->items[stk->size - 1];

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

finally:
        if (err) {
                set_error(error, err);
                log_dump(stk);
        }

        return item;
}

item_t *find_stack(stack *const stk, const item_t item, int *const error)
{
        assert(stk);
        int err = 0;

        item_t* found = nullptr;

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

        if (err) {
                fprintf(logs, "Can't pop item from invalid stk\n");
                goto finally;
        }

        if (stk->size == 0) {
                fprintf(logs, "Can't find in empty stk\n");
                err = STK_EMPTY_POP;
                goto finally;
        }

        for (int i = (int)stk->size - 1; i >= 0; i--) {
                if (stk->items[i] == item) {
                        found = &stk->items[i];
                        break;
                }
        }

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

finally:
        if (err) {
                set_error(error, err);
                log_dump(stk);
        }

        return found;
}

void push_stack(stack *const stk, const item_t item, int *const error) 
{
        assert(stk);
        int err = 0;

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

        if (err) {
                fprintf(logs, "Can't push to invalid stk\n");
                goto finally;
        }

        if (expandable(stk)) {
                size_t capacity = stk->capacity;
                capacity *= CAP_FACTOR;

$               (void *items = realloc_stack(stk, capacity);)
                if (!items) {
                        fprintf(logs, "Invalid stk expanding: %s\n", strerror(errno));
                        err = STK_BAD_ALLOC;
                        goto finally;
                }

                stk->capacity = capacity;
                stk->items    = (item_t *)items;
        }

        stk->items[stk->size++] = item;

#ifdef HASH_PROTECT
$       (stk->hash = hash_stk(stk);)
#endif /* HASH_PROTECT */

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

finally:
        if (err) {
                set_error(error, err);
                log_dump(stk);
        }
}

item_t pop_stack(stack *const stk, int *const error)
{
        assert(stk);
        int err = 0;

        item_t item = POISON;

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

        if (err) {
                fprintf(logs, "Can't pop item from invalid stk\n");
                goto finally;
        }

        if (stk->size == 0) {
                fprintf(logs, "Can't pop from an empty stk\n");
                err = STK_EMPTY_POP;
                goto finally;
        }
        if (shrinkable(stk)) {
                size_t capacity = stk->capacity;
                capacity /= CAP_FACTOR;

$               (void *items = realloc_stack(stk, capacity);)
                if (!items) {
                        fprintf(logs, "Invalid stk shrinking: %s\n", strerror(errno));
                        err = STK_BAD_ALLOC;
                        goto finally;
                }

                stk->items = (item_t *)items;
                stk->capacity = capacity;
        }

        item = stk->items[--stk->size];
        stk->items[stk->size] = POISON;

#ifdef HASH_PROTECT
$       (stk->hash = hash_stk(stk);)
#endif /* HASH_PROTECT */

#ifndef UNPROTECT
$       (err = verify_stk(stk);)
#endif /* UNPROTECT */

finally:
        if (err) {
                set_error(error, err);
                log_dump(stk);
        }

        return item;
}

stack *destruct_stack(stack *const stk)
{
        assert(stk);

        if (stk->items) {
#ifndef CANARY_PROTECT
            free(stk->items); stk->items = nullptr;
#else
            free(left_canary(stk->items)); stk->items = nullptr;
#endif /* CANARY_PROTECT */
        }

        stk->capacity     = 0;
        stk->size         = 0;

#ifdef HASH_PROTECT
        stk->hash         = 0;
#endif /* HASH_PROTECT */

#ifdef CANARY_PROTECT
        stk->left_canary  = 0;
        stk->right_canary = 0;
#endif /* CANARY_PROTECT */

        return stk;
}


/**
 * @brief Verifies an empty stk
 *
 * @param stk Stack to verify
 *
 * Tries to get all available information.
 * It doesn't change stk at all. 
 *
 * @return bit mask composed of invariant_err_t elemets
 */
static int verify_empty_stack(const stack *const stk)
{
        assert(stk);
        int vrf = 0x00000000;

        if (stk->items) 
                vrf |= INVALID_ITEMS;

        if (stk->capacity)
                vrf |= INVALID_CAPACITY;

        if (stk->size) 
                vrf |= INVALID_SIZE;

#ifdef HASH_PROTECT
        if (stk->hash)
                vrf |= INVALID_HASH;
#endif /* HASH_PROTECT */

#ifdef CANARY_PROTECT
        if (stk->left_canary)
                vrf |= INVALID_STK_LCNRY;
        if (stk->right_canary)
                vrf |= INVALID_STK_RCNRY;
#endif /* CANARY_PROTECT */

        return vrf;
}


/**
 * @brief Verifies stk
 *
 * @param stk Stack to verify
 *
 * Tries to get all available information.
 * It doesn't change stk at all. 
 *
 * @return bit mask composed of invariant_err_t elemets
 */
static int verify_stack(stack *const stk)
{
        assert(stk);
        int vrf = 0x00000000;

        if (stk->capacity > CAP_MAX || stk->capacity < INIT_CAP)
                vrf |= INVALID_CAPACITY;

        if (stk->size > stk->capacity) 
                vrf |= INVALID_SIZE;

#ifdef CANARY_PROTECT
        canary_t cnry = CANARY ^ (canary_t)stk->items;

        if (stk->items != nullptr) {

                if (*left_canary (stk->items) != cnry)
                        vrf |= INVALID_DATA_LCNRY;

                if (*right_canary(stk->items, stk->capacity) != cnry)
                        vrf |= INVALID_DATA_RCNRY;
        }

        if (stk->left_canary  != CANARY)
                vrf |= INVALID_STK_LCNRY;

        if (stk->right_canary != CANARY)
                vrf |= INVALID_STK_RCNRY;
#endif /* CANARY_PROTECT */

#ifdef HASH_PROTECT
        if (stk->items != nullptr) {
                if (stk->hash != hash_stk(stk))
                        vrf |= INVALID_HASH;
        }
#endif /* HASH_PROTECT */

        return vrf;
}

static inline item_t get_poison(const int byte) 
{
        item_t poison = 0;
        memset((void *)&poison, byte, sizeof(item_t));
        return poison;
}

static inline int expandable(const stack *const stk)
{
        assert(stk);
        return stk->capacity == stk->size;
}

static inline int shrinkable(const stack *const stk) 
{
        assert(stk);
        return stk->capacity / (CAP_FACTOR * CAP_FACTOR) + 1 >= stk->size && 
               stk->capacity > INIT_CAP;
}

static inline void set_error(int *const error, int value) 
{
        if (error)
                *error = value;
}

#ifdef CANARY_PROTECT
static inline canary_t *left_canary(void *const items)
{
        assert(items);
        return (canary_t *)((char *)items - sizeof(canary_t));
}
#endif /* CANARY_PROTECT */

#ifdef CANARY_PROTECT
static inline canary_t *right_canary(void *const items, const size_t capacity)  
{
        assert(items);
        return (canary_t *)((char *)items + 
                            (sizeof(item_t) * capacity) + sizeof(void *) -
                            (sizeof(item_t) * capacity) % sizeof(void *));
}
#endif /* CANARY_PROTECT */

static inline const char *indicate_err(int condition)
{
        if (condition)
                return "<font color=\"red\"><b>error</b></font>"; 
        else
                return "<font color=\"green\"><b>ok</b></font>";
}

/**
 * @brief Prints weird stk dump
 *
 * @param stk Stack to dump
 *
 * It is scary! Stay away from him...
 */
void dump_stack(stack *const stk)
{
        assert(stk);

        if (stk->items == nullptr) {

#ifndef NOLOG
                int vrf = verify_empty_stack(stk);
#endif /* NOLOG */

                fprintf(logs, "----------------------------------------------\n");
                fprintf(logs, " Empty stk: %s\n",                    indicate_err(vrf));
                fprintf(logs, " Size:     %10lu %s\n", stk->size,     indicate_err(vrf & INVALID_SIZE));
                fprintf(logs, " Capacity: %10lu %s\n", stk->capacity, indicate_err(vrf & INVALID_CAPACITY));
                fprintf(logs, " Address start: nullptr\n");
                fprintf(logs, "----------------------------------------------\n");

        } else {

#ifndef NOLOG
                int vrf = verify_stack(stk);
#endif /* NOLOG */

                fprintf(logs, "----------------------------------------------\n");
                fprintf(logs, " Stack: %s\n",                  indicate_err(vrf));
                fprintf(logs, " Size:     %15lu %s\n",      stk->size, indicate_err(vrf & INVALID_SIZE));
                fprintf(logs, " Capacity: %15lu %s\n",  stk->capacity, indicate_err(vrf & INVALID_CAPACITY));
                fprintf(logs, " Address start: 0x%lx\n", (size_t)stk->items);
                fprintf(logs, " Address   end: 0x%lx\n", (size_t)stk->items + 
                                        sizeof(item_t) * stk->capacity);
                fprintf(logs, "----------------------------------------------\n");

#ifdef HASH_PROTECT 
                fprintf(logs, " Hash       (hex): %8x %s\n", hash_stk(stk), 
                                                        indicate_err(vrf & INVALID_HASH));
                fprintf(logs, " Saved hash (hex): %8x\n", stk->hash);
                fprintf(logs, "----------------------------------------------\n");
#endif  /* HASH_PROTECT */

#ifdef CANARY_PROTECT
                fprintf(logs, " Left  stk canary(hex) = %lx %s\n", stk->left_canary,
                                        indicate_err(vrf & INVALID_STK_LCNRY));

                fprintf(logs, " Right stk canary(hex) = %lx %s\n", stk->right_canary,
                                        indicate_err(vrf & INVALID_STK_RCNRY));
                fprintf(logs, "----------------------------------------------\n");

                fprintf(logs, " Left  data canary(hex) =  %lx %s\n Address: 0x%lx\n", 
                               *left_canary(stk->items), 
                                indicate_err(vrf & INVALID_DATA_LCNRY),
                        (size_t)left_canary(stk->items));

                fprintf(logs, "\n");

                fprintf(logs, " Right data canary(hex) =  %lx %s\n Address: 0x%lx\n", 
                               *right_canary(stk->items, stk->capacity), 
                                indicate_err(vrf & INVALID_DATA_RCNRY),
                        (size_t)right_canary(stk->items, stk->capacity));
                fprintf(logs, "----------------------------------------------\n");
#endif  /* CANARY_PROTECT */

                for (size_t i = 0; i < stk->capacity; i++) {
                        if (stk->items[i] == POISON) {
                                fprintf(logs, "| 0x%.4lX stk[%7lu] = %p |\n", 
                                        sizeof(*stk->items) * i, i, "poison");
                        } else {
                                fprintf(logs, "| 0x%.4lX stk[%7lu] = %p |\n", 
                                        sizeof(*stk->items) * i, i, stk->items[i]);
                        }
                }

                fprintf(logs, "----------------------------------------------\n");

        }

        fprintf(logs, "\n\n\n");
}


