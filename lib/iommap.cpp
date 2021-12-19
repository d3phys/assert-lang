#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include <iommap.h>


int mmap_free(mmap_data *data) 
{
        assert(data);
        int err = munmap(data->buf, data->size);
        if (err)
                perror("Munmap failed");

        return errno; 
}

off_t get_size(const char *const file) 
{
        assert(file);
        errno = 0;

        struct stat buf = {};
        int err = stat(file, &buf);
        if (err)
                return -1;

        return buf.st_size;
}

int mmap_out(mmap_data *const data, const char *file)
{
        assert(file);
        errno = 0;

        const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        const int prot = PROT_READ | PROT_WRITE;
        char *bf = nullptr;

        int fd = -1;
        int err = 0;

        fd = open(file, O_RDWR | O_CREAT, mode); 
        if (fd == -1) {
                perror("Can't open file"); 
                goto cleanup;
        }

        err = ftruncate(fd, (off_t)data->size);
        if (err == -1) {
                perror("Can't truncate file"); 
                goto cleanup;
        }

        bf = (char *)mmap(nullptr, data->size, prot, MAP_SHARED, fd, 0);
        if (bf == MAP_FAILED) {
                perror("Can't map file");
                goto cleanup;
        }

        data->buf = bf;

cleanup:
        if (fd > 0)
                close(fd);

        return errno;
}

int mmap_in(mmap_data *const data, const char *file)
{
        assert(file);
        assert(data);

        errno = 0;

        const int prot = PROT_READ | PROT_WRITE;
        char *bf = nullptr;
        int fd = -1;
        
        off_t fsz = get_size(file);
        if (fsz == -1) {
                perror("Can't get the file size");
                goto cleanup;
        }

        if (fsz == 0) {
                fprintf(stderr, "File is empty: %s\n", file);
                errno = 1;
                goto cleanup;
        }

        fd = open(file, O_RDWR); 
        if (fd <= 0) {
                perror("Mmap can't open file");
                goto cleanup;
        }

        bf = (char *)mmap(nullptr, (size_t)(fsz + 1), prot, MAP_PRIVATE, fd, 0);
        if (bf == MAP_FAILED) {
                perror("Can't map file");
                goto cleanup;
        }

        bf[fsz] = '\0';

        data->size = (size_t)fsz + 1; 
        data->buf  = bf;

cleanup:
        if (fd > 0)
                close(fd);

        return errno;
}

