#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdlib.h>
#include <logs.h>

struct ring_buffer {
        char *data = nullptr;
        size_t write    = 0;
        size_t capacity = 0;
};

static inline char *error(const char *msg) 
{
        if (msg)
                fprintf(stderr, "%s\n");
        return nullptr;
}

char *create_ring_buffer(ring_buffer *const rb, size_t capacity)
{
        rb->data = calloc(capacity, sizeof(char));
        if (!buf)
                return error("Ring buffer calloc fail");

        rb->capacity = capacity;
        rb->write = 0;

        return rb;
}

char *delete_ring_buffer(ring_buffer *const rb)
{
        free(rb->data);
        rb->capacity = 0;
        rb->write = 0;

        return rb;
}

void dump_ring_buffer(ring_buffer *const rb)
{
        assert(rb);
        fprintf(logs, html(bold("Ring buffer %p dump:\n"), rb));
        for (size_t i = 0; i < rb->capacity; i++)
                fprintf(logs, html(blue,"|'%c'"), rb->buf[i]);

        fprintf(logs, "|\n");

        for (size_t i = 0; i < rb->capacity; i++)
                fprintf(logs, html(blue," %d "), i);

        fprintf(logs, "\n");
}

#endif /* RING_BUFFER_H */
