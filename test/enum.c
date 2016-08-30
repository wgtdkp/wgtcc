#include "test.h"


void test(void)
{
    typedef enum {
        AA,
        BB = 3,
        CC = 3,
        DD,
    } idtype_t;
    
    expect(AA, 1);
    expect(BB, 3);
    expect(CC, 4);
    expect(DD, 5);
    
}

int main(void)
{
    test();
    return 0;
}
