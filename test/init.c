#include "test.h"

void test1(void)
{
    typedef struct {
        struct {
            char a;
            char b;
        } d;
        char c;
    } foo_t;
    foo_t foo = {.d.a = 1};
}

int main(void)
{
    test1();
    return 0;
}
