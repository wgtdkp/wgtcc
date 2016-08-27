#include "test.h"

#include <stddef.h>

char g_p[] = "hello\x00000 world";

wchar_t g_arr[] = L"1234567890987654321";


void test1(void)
{
    const char* p = g_p;
    char arr[] = "1234567890987654321";
    printf("sizeof(g_p):%d\n", sizeof(g_p));
    expect(8, sizeof(p));
    expect(20, sizeof(arr));
    printf("%s\n", p);
    printf("%s\n", arr);
}

int main(void)
{
    test1();
    return 0;
}
