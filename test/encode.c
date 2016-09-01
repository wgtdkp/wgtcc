#include <stdio.h>
#include <stddef.h>

void test()
{
    const int* str = L"hello world";
    printf("%d\n", sizeof("\x00"));
}

int main()
{
    test();
    return 0;
}
