#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdio.h>

struct array {
        size_t capacity  = 0;
        size_t size      = 0;
        size_t item_size = 0;

        void *data = nullptr;
};

void *array_push   (array *const arr, void *item, size_t item_size);
void *array_create (array *const arr, size_t item_size);
void *array_extract(array *const arr, size_t item_size);

void free_array(array *const arr, size_t item_size);

void dump_array(array *const arr, size_t item_size, 
                void (* print)(void *) = nullptr);

void array_string(void *item);
void array_size_t(void *item);

#endif /* ARRAY_H */

