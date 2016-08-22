#include <stdio.h>
#include "test.h"

#define LINE 125

#line LINE

void test1(void)
{
    printf("__FILE__: %s\n", __FILE__);
    printf("__LINE__: %d\n", __LINE__);
}


int main(void)
{
    test1();
    return 0;
}
