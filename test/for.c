//#include "test.h"

#include <stdio.h>

#if 1
#define expect(a, b)                                        \
    if ((a) != (b)) {                                       \
        printf("error:%s:%s:%d: failed, %d != %d\n",        \
                __FILE__, __func__, __LINE__, (a), (b));    \
    };
#else
#define expect(a, b)
#endif

void test_decl(void)
{
    int sum = 0;
    for (int i = 0; i < 7; i++) {
        sum += i;
    }
    expect(28, sum);

    int i = 0;
    for (; i < 7; i++) {
        sum += i;
    }
    expect(56, sum);

    for (i = 0; i < 7; i++) {
        sum += i;
    }
    expect(84, sum);
}




int main(void)
{
    test_decl();
    return 0;
}
