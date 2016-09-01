#include "test.h"

void test1()
{
    int* p = (int*)4;
    expect(0, p == NULL);
    expect(1, p - 1 == NULL);
}

void test_arithm()
{
    int a;
    unsigned int b;
    float c, float d;
    int* p = &a;
    unsigned int* q = &b;
    printf("%d\n", p - q);
}


int main()
{
    test1();
    return 0;
}
