// @wgtdkp: passed

#include "test.h"

void test1()
{
    typedef struct {
        unsigned char a: 1;
        unsigned char b: 8;
        unsigned char c: 3;
        unsigned int d: 24;
    } foo_t;
    expect(8, sizeof(foo_t));
    //printf("%d\n", sizeof(foo_t));
}

void test2()
{
    typedef struct {
        unsigned char a: 1;
        unsigned char b: 3;
        unsigned char: 8;
        unsigned char c: 3;
        unsigned int d: 24;
    } foo_t;
    expect(8, sizeof(foo_t));
    //printf("%d\n", sizeof(foo_t));
}

void test3()
{
    typedef struct {
        unsigned short a: 6;
        unsigned short: 0;
        unsigned char b: 1;
    } foo_t;
    expect(2, sizeof(foo_t));
    //printf("%d\n", sizeof(foo_t));
}

void test4()
{
    typedef struct {
        unsigned short a: 8;
        unsigned int b: 9;
    } foo_t;
    expect(4, sizeof(foo_t));
}

void test5()
{
    typedef union {
        struct {
            unsigned long a: 15;
            unsigned int b: 20;
        };
        unsigned long c;
    } foo_t;
    expect(8, sizeof(foo_t));
    //printf("%d\n", sizeof(foo_t));
}

void test6()
{
    typedef struct {
        unsigned short a: 8;
        unsigned short b: 9;
    } foo_t;
    expect(4, sizeof(foo_t));
    //printf("%d\n", sizeof(foo_t));
}

void test7()
{
    typedef struct {
        unsigned int a: 8;
        unsigned short b: 9;
    } foo_t;
    expect(4, sizeof(foo_t));
    //printf("%d\n", sizeof(foo_t));
}

int main()
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
