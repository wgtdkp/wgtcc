#include <stdio.h>


const char* func = __func__;

int main(void)
{
    int c = 5;


    printf("%s\n", func);
    return 0;
}

