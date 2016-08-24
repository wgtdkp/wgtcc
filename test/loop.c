#include "test.h"

void test1(void)
{
    int i = 0, sum = 0;
    while (i < 10) {
        sum += i;
        ++i;
    }
    expect(sum, 45);
}

void test2(void)
{
    int i = 0, sum = 0;
    do {
        sum += i;
        ++i;
    } while (i < 10);
    expect(sum, 45);
}

void test3(void)
{
    int sum = 0;
    for (int i = 0; i < 10; ++i)
        sum += i;

    expect(sum, 45);
}

int main(void)
{
    test1();
    test2();
    test3();
    return 0;
}

