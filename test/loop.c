// @wgtdkp: passed

#include "test.h"

void test1()
{
    int i = 0, sum = 0;
    while (i < 10) {
        sum += i;
        ++i;
    }
    expect(sum, 45);
}

void test2()
{
    int i = 0, sum = 0;
    do {
        sum += i;
        ++i;
    } while (i < 10);
    expect(sum, 45);
}

void test3()
{
    int sum = 0;
    for (int i = 0; i < 10; ++i)
        sum += i;

    expect(sum, 45);
}

int main()
{
    test1();
    test2();
    test3();
    return 0;
}

