#include "test.h"


static void test_character_constant(void)
{
    expect(4, sizeof('a'));
    expect(2, sizeof(u'a'));
    expect(4, sizeof(U'a'));
    expect(4, sizeof(L'a'));
}

static void test_string_literal(void)
{
    printf("%u\n", sizeof(u"hello world"));
}


int main(void)
{
    test_character_constant();
    test_string_literal();
    return 0;
}
