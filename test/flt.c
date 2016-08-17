#include <stdio.h>

int main(void)
{
    double d = 4.5f;
    printf("%ld\n", *(long*)&d); 
    return 0;
}
