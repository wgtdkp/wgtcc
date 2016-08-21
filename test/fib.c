#include <stdio.h>


int fib(int n)
{
    int a = 0, b = 1, c = 0;
    for (int i = 0; i < n; i++) {
        a = b + c;
        c = b;
        b = a;
    }
    return a;
}

int main(void)
{
    return 0;
}