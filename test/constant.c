#include "test.h"

void test1(void)
{
    int c = '\77';
    printf("%x\n", c);
}

int main(void)
{
    test1();
    return 0;
}
