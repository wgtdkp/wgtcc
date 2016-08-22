#include <stdio.h>
#include <stddef.h>

void test(void)
{
    const int* str = L"hello world";
    printf("%d\n", sizeof("\x00"));
}

int main(void)
{
    test();
    return 0;
}
