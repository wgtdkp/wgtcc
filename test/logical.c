#include "test.h"

void test1(void)
{
    int a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b);
}

void test2(void)
{
    char a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b); 
}

void test3(void)
{
    float a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b);
}

void test3(void)
{
    double a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b);
}

int main(void)
{
    test1();
    test2();
    test3();
    return 0;
}