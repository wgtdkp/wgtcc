#include <stdio.h>


void test()
{
    typedef struct {
        int;
        int;
        int c;
    } foo_t;
    foo_t foo = {3};

    printf("%d\n", sizeof(foo));
}


int main()
{
    test();
    return 0;
}
