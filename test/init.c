#include <stdio.h>

static int a = (int)((int*)0 + 4);

int main(void)
{
    static int d = 44;
    static int* p = &d;
    return 0;
}
