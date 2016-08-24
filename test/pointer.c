#include "test.h"

void test1(void)
{
    int* p = (int*)4;
    expect(0, p == NULL);
    expect(1, p - 1 == NULL);
}


int main(void)
{
    test1();
    return 0;
}
