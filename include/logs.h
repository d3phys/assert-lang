#ifndef LOGS_H
#define LOGS_H

#include <stdio.h>

extern FILE *logs;

enum ascii_colors {
        ASCII_red    = 1,
        ASCII_green  = 2,
        ASCII_yellow = 3,
        ASCII_blue   = 4,
        ASCII_white  = 7,
};

char *local_time(const char *const fmt);

#define ascii(color, fmt) "\u001b[3%dm" fmt "\u001b[0m", ASCII_##color

#define html(color, fmt) "<font color=\"" #color "\">" fmt "</font>"

#define bold(fmt)   "<b>" fmt "</b>"
#define italic(fmt) "<i>" fmt "</i>"

#define $(code) fprintf(logs, "%s: %s\n", __PRETTY_FUNCTION__, #code); code

#define $$ fprintf(logs, "%s: %d\n", __PRETTY_FUNCTION__, __LINE__);

#define calloc(num, size)  calloc(num, size); \
        fprintf(logs, "%d:%s: calloc(" #num ", " #size") --> %lu bytes\n", __LINE__, __PRETTY_FUNCTION__, size * num);

#define realloc(ptr, size) realloc(ptr, size); \
        fprintf(logs, "%d:%s: realloc(" #ptr ", " #size") --> %lu bytes\n", __LINE__, __PRETTY_FUNCTION__, size);

#define free(ptr) free(ptr); fprintf(logs, "%d:%s: free(" #ptr ")\n", __LINE__, __PRETTY_FUNCTION__);

#endif /* LOG_H */
