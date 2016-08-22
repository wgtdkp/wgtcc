#include "test.h"

void test(void)
{
    char ch[] = "hello world";
    printf("%s, %d\n", ch, sizeof(ch));
}




int main(void)
{
    test();
    return 0;
}
