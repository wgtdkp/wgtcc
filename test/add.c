#include "test.h"

void test1()
{
    unsigned a, b;
    a = 2;
    b = 3;
    a = -b;
    printf("%d\n", a);
}

int main()
{
    test1();
    return 0;
}
