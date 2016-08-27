#include "test.h"

static void test1(void)
{  
    typedef struct {
        union {
            struct {
                char a;
                char b;
            };
            char d;
            char e;
        };
        char g;
        char h;
    } foo_t;
    foo_t foo = {.d = 1, 2, 3};
    expect(1, foo.a);
    expect(0, foo.b);
    expect(1, foo.d);
    expect(1, foo.e);
    expect(2, foo.g);
    expect(3, foo.h);
}

static void test2(void)
{
    typedef struct {
        char a;
        float b;
    } bar_t;

    typedef struct {
        char a;
        char b;
    } bar2_t;

    typedef struct {
        bar_t bar;
        char c;
        char d;
    } foo_t;
    
    bar_t b = {1, 2};
    bar2_t b2 = b;
    foo_t f = {b, 1, 2};

    expect(1, b.a);
    expect(2, b.b);
    expect(1, b2.a);
    expect(2, b2.b);
}

static void test_array(void)
{
    int arr[3] = {1, 2, 3};
    int arr1[] = {[0] = 1, 2, [0] = 2};
    typedef struct {
        int arr[4];
        int c;
    } foo_t;

    foo_t foo = {.arr[1] = 2, 3, 4, 5};
    int c = foo.arr[0];
    expect(2, foo.arr[1]);
    expect(3, foo.arr[2]);
    expect(4, foo.arr[3]);
    expect(5, foo.c);

    expect(12, sizeof(arr));
    expect(8, sizeof(arr1));

    int arr2[2][3] = {[0][1] = 1, 2, 3,};
    expect(24, sizeof(arr2));
    expect(1, arr2[0][1]);
    expect(2, arr2[0][2]);
    expect(3, arr2[1][0]);
}


static void test_literal(void)
{
    char arr[] = "123456789";
    expect(10, sizeof(arr));
}

static void test_dup(void)
{
    int arr[] = {[0] = 1, 2, 3, 4, 5, [0] = 5};
    expect(5, arr[0]);
}


int main(void)
{
    test1();
    test2();
    test_array();
    test_literal();
    test_dup();
    return 0;
}


