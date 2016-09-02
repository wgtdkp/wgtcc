#include "test.h"


void test1()
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
