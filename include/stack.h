/**
 * @file 
 * @brief  Stack implementation
 * @author d3phys
 * @date   07.10.2021
 */

#ifndef STACK_H_
#define STACK_H_

#include <stdint.h>
#include <stdlib.h>


typedef void * item_t;

#define UNPROTECT

#ifdef CANARY_PROTECT
typedef uint64_t canary_t;
const   uint64_t CANARY = 0xCCCCCCCCCCCCCCCC;
#endif /* CANARY_PROTECT */

#ifdef HASH_PROTECT
typedef uint32_t hash_t;
const   uint32_t SEED = 0xDED32BAD;
#endif /* HASH_PROTECT */

/**
 * @brief Stack structure
 */
struct stack {

#ifdef CANARY_PROTECT
        canary_t left_canary = 0;
#endif /* CANARY_PROTECT */

        item_t *items         = nullptr;
        size_t capacity       = 0;
        size_t size           = 0;

#ifdef HASH_PROTECT
        hash_t hash           = 0;
#endif /* HASH_PROTECT */

#ifdef CANARY_PROTECT
        canary_t right_canary = 0;
#endif /* CANARY_PROTECT */

};

/**
 * @brief Stack error codes 
 */
enum stack_error_t {
        STK_BAD_ALLOC = 0xBADA110C, 
        STK_OVERFLOW  = 0x0000F112, 
        STK_INVALID   = 0xABADBABE,
        STK_EMPTY_POP = 0x00000E11,
};

/**
 * @brief Verification error flags 
 */
enum invariant_err_t {
        INVALID_CAPACITY   = 1 << 0,
        INVALID_SIZE       = 1 << 1,
        INVALID_ITEMS      = 1 << 2,
        INVALID_HASH       = 1 << 3,
        INVALID_DATA_LCNRY = 1 << 4,
        INVALID_DATA_RCNRY = 1 << 5,
        INVALID_STK_LCNRY  = 1 << 6,
        INVALID_STK_RCNRY  = 1 << 7,
};

#define log_dump(_stk)               \
        do {                         \
                fprintf(logs, "Stack dump\n"); \
                dump_stack(_stk);    \
        } while (0)

/**
 * @brief Dumps stk
 *
 * @param stk Stack to dump
 *
 * It prints stk dump to a log file. 
 * If log file is empty stderr stream is used.
 * There are a lot of useful information inside.
 */
void dump_stack(stack *const stk);

/**
 * @brief Stack constructor
 *
 * @param[out] stk      Stack to create
 * @param[out] error    Error proceeded
 *
 * It is the manual way to create stk. 
 * In case of an error, nothing happens to the stk.
 * Do not use other ways. 
 */
stack *construct_stack(stack *const stk, int *const error = nullptr);

/**
 * @brief Stack destructor 
 *
 * @param stk Stack to destroy 
 *
 * It takes any bullshit and makes clear stk.
 * After destruction stk can be created again.
 */
stack *destruct_stack(stack *stk);

/**
 * @brief Pushes item to stk 
 *
 * @param stk        Stack push to
 * @param item       Item to push
 * @param[out] error Error proceeded
 *
 * This function works with valid and not empty stk only.
 * It rescales stk if there are no empty space availible. 
 * In case of an error, nothing happens to the stk.
 */
void push_stack(stack *const stk, const item_t item, int *const error = nullptr);

/**
 * @brief Pops item from stk 
 *
 * @param stk        Stack pop from 
 * @param[out] error Error proceeded
 *
 * This function works with valid and not empty stk only.
 * It rescales stk if there are a lot empty space availible. 
 * In case of an error, nothing happens to the stk.
 *
 * @return 'Popped' item
 */
item_t pop_stack(stack *const stk, int *const error = nullptr);

/**
 * @brief Gives top stk item.
 *
 * @param stk        Stack check from 
 * @param[out] error Error proceeded
 *
 * This function works with valid and not empty stk only.
 * In case of an error, nothing happens to the stk.
 *
 * @return 'Top' stk item
 */
item_t check_stack(stack *const stk, int *const error = nullptr);

item_t *find_stack(stack *const stk, const item_t item, int *const error = nullptr);

item_t *data_stack(stack *const stk, int *const error = nullptr);

item_t    top_stack(stack *const stk, int *const error = nullptr);
item_t bottom_stack(stack *const stk, int *const error = nullptr);


#endif /* STACK_H_ */


