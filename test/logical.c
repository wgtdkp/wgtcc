#include "test.h"

void test1()
{
    int a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b);
}

void test2()
{
    char a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b); 
}

void test3()
{
    float a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b);
}

void test3()
{
    double a, b;
    a = 3;
    b = 4;
    expect(1, a && b);
    expect(1, a || b);
    expect(0, !a || !b);
}

int main()
{
    test1();
    test2();
    test3();
    return 0;
}