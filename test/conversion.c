// Copyright 2012 Rui Ueyama. Released under the MIT license.
// @wgtdkp: passed

#include "test.h"
#include <stdbool.h>

static void test_bool() {
    bool v = 3;
    expect(1, v);
    v = 5;
    expect(1, v);
    v = 0.5;
    expect(1, v);
    v = 0.0;
    expect(0, v);
}

static void test_float() {
    double a = 4.0;
    float b = a;
    expectf(4, b);
}

int main() {
    test_bool();
    test_float();
    return 0;
}
