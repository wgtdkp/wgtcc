#include <stdio.h>
#include "test.h"

#define LINE 125

//#line LINE

#define PRINT_LINE \
    printf("%d\n", __LINE__);


void test1(void)
{
    printf("__FILE__: %s\n", __FILE__);
    printf("__LINE__: %d\n", __LINE__);
}


int main(void)
{
    PRINT_LINE;
    test1();
    return 0;
}
