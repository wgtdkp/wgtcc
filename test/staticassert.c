// Copyright 2012 Rui Ueyama. Released under the MIT license.
// @wgtdkp: passed

#include "test.h"

int main() {
    _Static_assert(1, "fail");

    struct {
        _Static_assert(1, "fail");
    } x;
    return 0;
}
