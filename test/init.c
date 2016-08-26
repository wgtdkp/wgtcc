#include "test.h"

void test1(void)
{
    typedef struct {
        union {
            struct {
                char a;
                char b;
            };
            char d;
            char e;
        };
        char g;
        char h;
    } foo_t;
    foo_t foo = {.d = 1, 2, 3};
    printf("%d\n", foo.a);
    printf("%d\n", foo.b);
    printf("%d\n", foo.d);
    printf("%d\n", foo.e);
    printf("%d\n", foo.g);
    printf("%d\n", foo.h);
}

int main(void)
{
    test1();
    return 0;
}
