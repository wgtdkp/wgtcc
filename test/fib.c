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
    double j = 5;
    //int d = j < 128;
    //printf("%d\n", d);
    for (j = 0; j < 128; ++j) {
        //printf("fib(%d): %d\n", i, fib(i));
        printf("sum(%d): %d\n", (int)j, sum(j));
    }
    return 0;
}