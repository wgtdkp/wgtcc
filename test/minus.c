#include "test.h"

void test1(void)
{
    double a, b;
    a = 5.0;
    b = 0 - a;
    printf("%f\n", b);
}

void test2(void)
{
    char a, b;
    a = -1;
    b = -a;
    printf("%d\n", b);
}


int main(void)
{
    test1();
    test2();
    return 0;
}
