#include <stdio.h>
#include <time.h>
#include <assert.h>

#include <logs.h>

FILE *logs = nullptr;

#ifdef LOG_FILE

__attribute__((constructor(101)))
static void init()
{ 
        logs = fopen(LOG_FILE, "w");
        if (logs) {
                int err = setvbuf(logs, nullptr, _IONBF, 0);
                if (!err) {
                        fprintf(logs, "<pre>\n");
                        return;
                }

                fclose(logs);
        }

        logs = stderr;
}

#else

__attribute__((constructor(101)))
static void init() { logs = stderr; }

#endif /* LOG_FILE */ 
       
/**
 * @brief  Gets local time
 * @param  fmt  time format
 *
 * Function uses local static buffer with a constant length.
 * It is designed to be called multiple times.
 *
 * @return Local time as a formatted string.
 */
char *local_time(const char *const fmt)
{
        assert(fmt);

        static const size_t buf_size = 0xFF;
        static char str_tm[buf_size] = {0};

        static time_t t = time(nullptr);
        if (t == -1)
                return nullptr;

        static tm *lt = localtime(&t);
        strftime(str_tm, buf_size, fmt, lt);

        return str_tm;
}

__attribute__((destructor))
static void kill()
{
        fclose(logs);
}


