#include "test.h"

void test1()
{
    int a = 32, b = 4;
    expect(8, a / b);
}

int main()
{
    test1();
    return 0;
}
