#include "test.h"


static void test_character_constant()
{
    expect(4, sizeof('a'));
    expect(2, sizeof(u'a'));
    expect(4, sizeof(U'a'));
    expect(4, sizeof(L'a'));
}

static void test_string_literal()
{
    expect(12, sizeof("hello world"));
    expect(12, sizeof(u8"hello world"));
    expect(24, sizeof(u"hello world"));
    expect(48, sizeof(U"hello world"));
    expect(48, sizeof(L"hello world"));
    printf("%u\n", sizeof("hello\e world"));
}


int main()
{
    test_character_constant();
    test_string_literal();
    return 0;
}
