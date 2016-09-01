#include <stdio.h>
#include "test.h"

#define LINE 125

//#line LINE

#define PRINT_LINE \
    printf("%d\n", __LINE__);


void test1()
{
    printf("__FILE__: %s\n", __FILE__);
    printf("__LINE__: %d\n", __LINE__);
}


int main()
{
    PRINT_LINE;
    test1();
    return 0;
}
