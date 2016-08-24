#include "test.h"


void test1(void)
{
    int a, b;
    b = 3;
    a = !b;
    b = !a;
    expect(0, a);
    expect(1, b);
}

void test2(void)
{
    float a, b;
    int c, d;
    a = 3;
    b = 0;
    c = !a;
    d = !b;
    expect(c, 0);
    expect(d, 1);
}

void test3(void)
{
    int a, b;
    b = 0;
    a = ~b;
    expect(a, -1);
}

void test4(void)
{
    unsigned a, b;
    b = 0;
    a = ~b;
    expect(a, -1);
}

int main(void)
{
    test1();
    test2();
    test3();
    test4();
    return 0;
}
