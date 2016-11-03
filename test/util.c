#include <stdio.h>

static void test() {
    int c = 11;
    int arr[c];
    arr[3] = 5;
    return;
}

int main() {
    test();
    return 0;
}
