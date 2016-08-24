#include "test.h"

void test1(void)
{
    int a = 32, b = 4;
    expect(8, a / b);
}

int main(void)
{
    test1();
    return 0;
}
