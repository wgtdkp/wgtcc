#include <stdio.h>


static char arr[];


int main(void)
{
    static int d = 44;
    static int* p = &d;
    return 0;
}
