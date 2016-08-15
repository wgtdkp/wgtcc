#include <stdio.h>


static int* p = 0;

int main(void)
{
    long i = p;
    printf("%p\n", p);
    return 0;
}