#ifndef IOMMAP_H
#define IOMMAP_H

#include <sys/types.h>
#include <stddef.h>

struct mmap_data {
        char   *buf = nullptr;
        size_t size = 0;
};

off_t get_size(const char *const file);

/*
 * Frees memory after mmap_in or mmap_out.
 */
int mmap_free(mmap_data *data);

/*
 * Maps file into memory. 
 * This is simply mmap() wrapper with usefull params. 
 *
 * Important! Function does not set 'data.size'.
 * That's why 'data' argument must contain 'size'. 
 */
int mmap_out(mmap_data *const data, const char *file);

/*
 * Maps file into memory. 
 * This is simply mmap() wrapper with usefull params. 
 *
 * The usage of 'data' is the same as after calloc(), 
 * but remember to free memory with mmap_free().
 * Note! Function sets 'data.size'.
 *
 * It's comfortable to use it for mapping data 
 * that you want to read.
 */
int mmap_in(mmap_data *const data, const char *file);


#endif /* IOMAP_H */
