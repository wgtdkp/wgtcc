//#include "test.h"

#define swap(x) ((unsigned short int) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

void test(void)
{
    int x;
    x = swap(x);
}

int main(void)
{

    return 0;
}