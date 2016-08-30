#include "test.h"
#include <uchar.h>
#include <wchar.h>
#include <stddef.h>

static void test()
{
    const wchar_t* p = L"ab";
    char* q = "ab";
    int c = p - q;
}

int main(void)
{
    //test_char();
    //test_string();
    test();
    return 0;
}