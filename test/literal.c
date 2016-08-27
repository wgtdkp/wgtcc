#include "test.h"

const char* g_p = "hello world";

char g_arr[] = "1234567890987654321";


void test1(void)
{
    const char* p = g_arr;
    char arr[] = "1234567890987654321";
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
