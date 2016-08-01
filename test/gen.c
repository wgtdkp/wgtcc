#include <stdio.h>


struct bar_t {
    int a;
    int b;
    int c;
    int d;
} bar;


int g = 0;
struct bar_t foo(void)
{
    int a = 0;
    char b = 1;
    int c = 2;
    char d = 3;
    int e = 4;

    {
        char f = 5;
        int g = 6;
        long long h = 7;
    }

    {
        char f = 8;
        int g = 9;
        long long h = 10;
    }

    {
        char f = 5;
        int g = 6;
        long long h = 7;
    }

    {
        char f = 8;
        int g = 9;
        long long h = 10;
    }

    {
        char f = 5;
        int g = 6;
        long long h = 7;
    }

    {
        char f = 8;
        int g = 9;
        long long h = 10;
    }

    {
        char f = 5;
        int g = 6;
        long long h = 7;
    }

    {
        char f = 8;
        int g = 9;
        long long h = 10;
    }


    return bar;
}


int* gp;
int hello(int par)
{
    int a, d;
    int* b;
    int** c;
    //a = 3;
    a = *b;
    a = *gp;
    char bo = (char)a;
    a = -d;
    //printf("%d\n", *b);
    return 0;
}


void test_param(int a, int b, int c, int d,
        int e, int f, int g, int h, int i, int j , int k)
{

    a = 0;
    b = 1;
    c = 2;
    d = 3;
    e = 4;
    f = 5;
    g = 6;
    h = 7;
    i = 8;
    j = 9;
    k = 10;

    return;
}


void test_ld(long double ld)
{
    ld = 45.0;
}

void test_arr(int arr[100])
{

}


int world(void)
{
    int p1, p2, p3, p4, p5;
    test_param(10, 11, 12, 13, 14, 15, p1, p2, p3, p4, p5);

    long double ld;
    test_ld(ld);

    int arr[100];
    test_arr(arr);

    return 0;
}


