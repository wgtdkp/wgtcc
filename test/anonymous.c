#include <stdio.h>


void test(void)
{
    typedef struct {
        int;
        int;
        int c;
    } foo_t;
    foo_t foo = {3};

    printf("%d\n", sizeof(foo));
}


int main(void)
{
    test();
    return 0;
}
