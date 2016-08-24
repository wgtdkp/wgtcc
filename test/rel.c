#include "test.h"

void test1(void)
{
    int a, b;
    a = -1;
    a == -1;
    //printf("%d\n", (a == -1));
}

void test2(void)
{
    int a = 4, b = 5;
    expect(1, a < b);
    expect(1, b > a);
}

int main(void)
{
    test1();
    test2();
    return 0;
}
