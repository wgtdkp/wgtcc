#include "test.h"
#include <stddef.h>

void test1(void)
{
    typedef struct {
        int a;
        struct {
            int b;
            int c;
        } d;
    } foo_t;

    int c = offsetof(foo_t, d.c);
}
