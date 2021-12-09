#include <array.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <logs.h>
#include <array.h>
#include <string.h>

static const size_t INIT_CAPACITY = 2;

static inline int validate_size(array *const arr, size_t item_size);

void array_string(void *item)
{
        assert(item);
        if (*(char **)item)
                fprintf(logs, "%s [%p]", *(char **)item, *(char **)item);
}

void array_size_t(void *item)
{
        assert(item);
        fprintf(logs, "%lu", *(size_t *)item);
}


void free_array(array *const arr, size_t item_size)
{
        assert(arr);

        if (validate_size(arr, item_size))
                return;

        free(arr->data);

        arr->size      = 0;
        arr->capacity  = 0;
        arr->item_size = 0;

        arr->data = nullptr;
}

void *realloc_array(array *const arr, size_t capacity, size_t item_size)
{
        assert(arr);

        if (validate_size(arr, item_size))
                return nullptr;

        if (!capacity)
                capacity = INIT_CAPACITY;

        void *data = realloc(arr->data, capacity * item_size);
        if (!data) {
                perror("Can't realloc array");
                return nullptr;
        }

        memset(data + arr->size * item_size, 0, 
              (capacity - arr->size) * item_size);

        arr->capacity = capacity;
        arr->data     = data;

        return data;
}

void *array_push(array *const arr, void *item, size_t item_size)
{
        assert(arr);
        assert(item && item_size);

        if (validate_size(arr, item_size))
                return nullptr;

        void *data = arr->data;
        if (arr->size >= arr->capacity) {
                data = realloc_array(arr, arr->capacity * 2, item_size);
                if (!data)
                        return nullptr;
        }

        memcpy((char *)data + item_size * arr->size, item, item_size);
        return (char *)data + item_size * arr->size++;
}

void *array_create(array *const arr, size_t item_size)
{
        assert(arr);
        assert(item_size);

        if (validate_size(arr, item_size))
                return nullptr;

        void *data = arr->data;
        if (arr->size >= arr->capacity) {
                data = realloc_array(arr, arr->capacity * 2, item_size);
                if (!data)
                        return nullptr;
        }

        return ((char *)data + arr->size++ * item_size);
}

void *array_extract(array *const arr, size_t item_size)
{
        assert(arr);

        if (validate_size(arr, item_size))
                return nullptr;

        if (!arr->capacity)
                arr->capacity = INIT_CAPACITY;

        void *data = realloc_array(arr, arr->size, item_size);
        if (!data)
                return nullptr;

        arr->size     = 0;
        arr->capacity = 0;
        arr->data = nullptr;

        return data;
}

static inline int validate_size(array *const arr, size_t item_size)
{
        assert(arr);
        if (!arr->item_size) {
                arr->item_size = item_size;
                return 0;
        }

        if (arr->item_size != item_size) {
                fprintf(logs, "<font color=red><b>"
                              "Invalid item size (%lu bytes) provided "
                              "to the array (%p) with item size %lu bytes\n"
                              "</b></font>", item_size, arr, arr->item_size);
                return 1;
        }

        return 0;
}

void dump_array(array *const arr, size_t item_size, 
                void (*print)(void *))
{
        assert(arr);

        if (validate_size(arr, item_size))
                return;

        fprintf(logs, "================================================\n"
                      "| <b>Array %p dump</b>                          \n"
                      "================================================\n", arr);

        for (size_t i = 0; i < arr->capacity; i++) {
                fprintf(logs, "------------------------------------------------\n"
                              "| %-4lu | ", i);

                fprintf(logs, "0x%-4x| ", i * item_size);
                if (print) print(arr->data + i * item_size);
                fprintf(logs, "\n");
        }

        fprintf(logs, "================================================\n"
                      "| Fill: %lu/%lu                                 \n"
                      "| Item size: %lu                                \n"
                      "| Data address: %p                              \n"
                      "| Allocated %lu bytes                           \n"
                      "| Address: %p                                   \n"
                      "================================================\n\n\n", 
                      arr->size, arr->capacity, arr->item_size, arr->data, 
                      arr->capacity * arr->item_size, arr);
}



