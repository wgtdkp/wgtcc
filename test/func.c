#include <stdio.h>
#include <stdarg.h>

int test(int a1, int a2, int a3, int a4, int a5, int a6, int a7, ...)
{
    va_list args;
    va_start(args, a7);
    int i1 = va_arg(args, int);
    va_end(args);

    return 0;
}

int testl(long a1, long a2, long a3, long a4, long a5, long a6, long a7, ...)
{
    va_list args;
    va_start(args, a7);
    int i1 = va_arg(args, int);
    va_end(args);

    return 0;
}
