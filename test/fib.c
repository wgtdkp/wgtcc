#include <stdio.h>


int fib(int n)
{
    int a = 0, b = 1, c = 0;
    int i = 0;
    for (; i < n; i++) {
        a = b + c;
        c = b;
        b = a;
    }
    return a;
}

int main(void)
{
    int i;
    for (i = 0; i < 10; i++) {
        printf("fib(%d): %d\n", i, fib(i));
    }
    return 0;
}