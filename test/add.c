#include "test.h"

void test1(void)
{
    unsigned a, b;
    a = 2;
    b = 3;
    a = -b;
    printf("%d\n", a);
}

int main(void)
{
    test1();
    return 0;
}
