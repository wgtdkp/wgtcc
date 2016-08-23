#include <stdio.h>


void test1(void)
{
    typedef struct {
        unsigned char a: 1;
        unsigned char b: 8;
        unsigned char c: 3;
        unsigned int d: 24;
    } foo_t;
    printf("%d\n", sizeof(foo_t));
}

void test2(void)
{
    typedef struct {
        unsigned char a: 1;
        unsigned char b: 3;
        unsigned char: 8;
        unsigned char c: 3;
        unsigned int d: 24;
    } foo_t;
    printf("%d\n", sizeof(foo_t));
}

void test3(void)
{
    typedef struct {
        unsigned short a: 8;
        unsigned char: 0;
        unsigned char b: 1;
    } foo_t;
    printf("%d\n", sizeof(foo_t));
}

void test4(void)
{
    typedef struct {
        unsigned short a: 8;
        unsigned int b: 9;
    } foo_t;
    printf("%d\n", sizeof(foo_t));
}


int main(void)
{
    typedef union {
        struct {
            unsigned int a: 8;
            unsigned short b: 7;
            unsigned int c: 10;
        };
    } foo_t;

    foo_t foo;
    printf("%ld\n", sizeof(foo_t));

    test1();
    test2();
    test3();
    test4();
    return 0;
}
