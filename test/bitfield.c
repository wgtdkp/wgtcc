#include "test.h"


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

void test5(void)
{
    typedef union {
        struct {
            unsigned long a: 15;
            unsigned int b: 20;
        };
        unsigned long c;
    } foo_t;
    printf("%d\n", sizeof(foo_t));
    //foo_t foo;
    //foo.c = 0;
    //foo.a = 33;
    //foo.b = 33;
    //printf("%lx\n", foo.a);   
    //printf("%lx\n", foo.b);   
    //printf("%lx\n", foo.c);    
}

void test6(void)
{
    typedef struct {
        unsigned short a: 8;
        unsigned short b: 9;
    } foo_t;
    printf("%d\n", sizeof(foo_t));
}

void test7(void)
{
    typedef struct {
        unsigned int a: 8;
        unsigned short b: 9;
    } foo_t;
    printf("%d\n", sizeof(foo_t));
    foo_t foo;
    
    memset((void*)&foo, 0, sizeof(foo_t));
    foo.a = 34;
    foo.b = 34;
    printf("%x\n", *((int*)&foo));
}

int main(void)
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    return 0;
}
