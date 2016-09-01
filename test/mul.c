#include "test.h"

static void test_int() {
    int s1, s2;
    s1 = -3;
    s2 = -4;

    expect(-12, s1 * 4);
    expect(12, s1 * -4);
    expect(12, s1 * s2);
    expect(2, s2 / -2);
    expect(1, s1 / -2);
    expect(0, s1 / s2);
}


int main(void) {
    test_int();
    return 0;
}


