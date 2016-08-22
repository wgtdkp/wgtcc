#include <stdio.h>

#define LINE 125

#line LINE

void test1(void)
{
    printf("__LINE__: %d\n", __LINE__);
}



int main(void)
{
    test1();
    return 0;
}
