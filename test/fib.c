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

int sum(int n)
{
    int sum = 0;
    int i;
    for (i = 0; i <= n; ++i) {
        sum += i;
    }
    return sum;
}

int main(void)
{
    unsigned char i;
    for (i = 0; i < 128; ++i) {
        //printf("fib(%d): %d\n", i, fib(i));
        printf("sum(%d): %d\n", i, sum(i));
    }
    return 0;
}