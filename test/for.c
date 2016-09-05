//#include "test.h"

#include <stdio.h>


void test_decl()
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




int main()
{
    test_decl();
    return 0;
}
