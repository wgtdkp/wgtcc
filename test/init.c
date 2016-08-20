#include <stdio.h>


static char arr[] = "12345";

int main(void)
{
    static int d = 44;
    static int* p = &d;
    return 0;
}
