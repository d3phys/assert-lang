#include <stdint.h>
#include <stdio.h>

uint64_t
__ass_print( uint64_t v)
{
    printf( "%ld\n", v);
    return 0;
}

uint64_t
__ass_scan( void)
{
    uint64_t tmp = 0;
    scanf( "%ld", &tmp);
    return tmp;
}

