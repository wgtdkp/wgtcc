// @wgtdkp: passed

#include "test.h"

void test1()
{
    double a, b;
    a = 3.5;
    b = 4.5;
    expect(15.75, a * b);
}

void test2()
{
    char a, b;
    a = -200;
    b = -20;
    printf("%d\n", (unsigned)a * b);
}

int main()
{
    test1();
    test2();
    return 0;
}
