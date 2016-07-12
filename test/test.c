#include <stdio.h>
#include <complex.h>
int main(void)
{
    printf("sizeof(short): %d\n", sizeof(short));
    printf("sizeof(int): %d\n", sizeof(int));
    printf("sizeof(long): %d\n", sizeof(long));
    printf("sizeof(long long): %d\n", sizeof(long long));
    printf("sizeof(float): %d\n", sizeof(float));
    printf("sizeof(double): %d\n", sizeof(double));
    printf("sizeof(long double): %d\n", sizeof(long double));
    printf("sizeof(double complex): %d\n", sizeof(double complex));
    printf("sizeof(long double complex): %d\n", sizeof(long double complex));
    return 0;
}