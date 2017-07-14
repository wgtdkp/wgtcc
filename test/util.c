#include "test.h"
int main() {
    struct S {
        unsigned a: 3;
        signed b: 3;
        int c: 3;
    };
    int a[2];
    a = 0;
    return 0;
}
