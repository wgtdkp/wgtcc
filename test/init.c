// Copyright 2012 Rui Ueyama. Released under the MIT license.
// @wgtdkp: passed

#include "test.h"

static void verify(int *expected, int *got, int len) {
    for (int i = 0; i < len; i++)
        expect(expected[i], got[i]);
}

static void verify_short(short *expected, short *got, int len) {
    for (int i = 0; i < len; i++)
        expect(expected[i], got[i]);
}

static void test_string() {
    char s[] = "abc";
    expect_string("abc", s);
    char t[] = { "def" };
    expect_string("def", t);
}

static void test_struct() {
    int we[] = { 1, 0, 0, 0, 2, 0, 0, 0 };
    struct { int a[3]; int b; } w[] = { { 1 }, 2 };
    verify(we, &w, 8);
}

static void test_primitive() {
    int a = { 59 };
    expect(59, a);
}

static void test_nested() {
    struct {
        struct {
            struct { int a; int b; } x;
            struct { char c[8]; } y;
        } w;
    } v = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, };
    expect(1, v.w.x.a);
    expect(2, v.w.x.b);
    expect(3, v.w.y.c[0]);
    expect(10, v.w.y.c[7]);
}

static void test_array_designator() {
    int v[3] = { [1] = 5 };
    expect(0, v[0]);
    expect(5, v[1]);
    expect(0, v[2]);

    struct { int a, b; } x[2] = { [1] = { 1, 2 } };
    expect(0, x[0].a);
    expect(0, x[0].b);
    expect(1, x[1].a);
    expect(2, x[1].b);

    struct { int a, b; } x2[3] = { [1] = 1, 2, 3, 4 };
    expect(0, x2[0].a);
    expect(0, x2[0].b);
    expect(1, x2[1].a);
    expect(2, x2[1].b);
    expect(3, x2[2].a);
    expect(4, x2[2].b);

    int x3[] = { [2] = 3, [0] = 1, 2 };
    expect(1, x3[0]);
    expect(2, x3[1]);
    expect(3, x3[2]);
}

static void test_struct_designator() {
    struct { int x; int y; } v1 = { .y = 1, .x = 5 };
    expect(5, v1.x);
    expect(1, v1.y);

    struct { int x; int y; } v2 = { .y = 7 };
    expect(7, v2.y);

    struct { int x; int y; int z; } v3 = { .y = 12, 17 };
    expect(12, v3.y);
    expect(17, v3.z);
}

static void test_complex_designator() {
    struct { struct { int a, b; } x[3]; } y[] = {
        [1].x[0].b = 5, 6, 7, 8, 9,
        [0].x[2].b = 10, 11
    };
    expect(0, y[0].x[0].a);
    expect(0, y[0].x[0].b);
    expect(0, y[0].x[1].a);
    expect(0, y[0].x[1].b);
    expect(0, y[0].x[2].a);
    expect(10, y[0].x[2].b);
    expect(11, y[1].x[0].a);
    expect(5, y[1].x[0].b);
    expect(6, y[1].x[1].a);
    expect(7, y[1].x[1].b);
    expect(8, y[1].x[2].a);
    expect(9, y[1].x[2].b);

    int y2[][3] = { [0][0] = 1, [1][0] = 3 };
    expect(1, y2[0][0]);
    expect(3, y2[1][0]);

    struct { int a, b[3]; } y3 = { .a = 1, .b[0] = 10, .b[1] = 11 };
    expect(1, y3.a);
    expect(10, y3.b[0]);
    expect(11, y3.b[1]);
    expect(0, y3.b[2]);
}

static void test_zero() {
    struct tag { int x, y; };
    struct tag v0 = (struct tag){ 6 };
    expect(6, v0.x);
    expect(0, v0.y);

    struct { int x; int y; } v1 = { 6 };
    expect(6, v1.x);
    expect(0, v1.y);

    struct { int x; int y; } v2 = { .y = 3 };
    expect(0, v2.x);
    expect(3, v2.y);

    struct { union { int x, y; }; } v3 = { .x = 61 };
    expect(61, v3.x);
}


static void test_typedef() {
    typedef int A[];
    A a = { 1, 2 };
    A b = { 3, 4, 5 };
    expect(2, sizeof(a) / sizeof(*a));
    expect(3, sizeof(b) / sizeof(*b));
}

static void test_excessive() {
#ifdef __8cc__
#pragma disable_warning
#endif
    // wgtcc treats exceeding elements as error
    char x1[3] = { 1, 2, 3/*, 4, 5*/ };
    expect(3, sizeof(x1));

    char x2[3] = "abcdefg";
    expect(3, sizeof(x2));
    expect(0, strncmp("abc", x2, 3));

#ifdef __8cc__
#pragma disable_warning
#endif
}

static void test1() {
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

static void test2() {
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
    bar_t b2 = b;
    foo_t f = {b, 1, 2};

    expect(1, b.a);
    expect(2, b.b);
    expect(1, b2.a);
    expect(2, b2.b);
}

static void test_array() {
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


static void test_literal() {
    char arr[] = "123456789";
    expect(10, sizeof(arr));
}

static void test_dup() {
    int arr[] = {[0] = 1, 2, 3, 4, 5, [0] = 5};
    expect(5, arr[0]);
}

static void test_struct_anonymous_1() {
    typedef struct {
        struct {
            int a;
            int b;
        };
        union {
            char c;
            int d;
        };
    } foo_t;
    // undesignated
    foo_t foo = {1, 2, 3};
    expect(1, foo.a);
    expect(2, foo.b);
    expect(3, foo.c);
    expect(3, foo.d);
    foo_t foo1 = {1, 2, .d = 3};
    expect(1, foo.a);
    expect(2, foo.b);
    expect(3, foo.c);
    expect(3, foo.d);
}

static void test_struct_anonymous_2() {
    typedef struct {
        union {
            int a;
            char b;
        };
        union {
            int c;
            long d;
        };
    } foo_t;
    foo_t foo = {1, 2};
    expect(1, foo.a);
    expect(1, foo.b);
    expect(2, foo.c);
    expect(2, foo.d);
}

static void test_struct_anonymous_complex() {
    typedef struct {
        char a;
        short b;
        int c;
        union {
            struct {
                union {
                    int d;
                    char e;
                };
                struct {
                    long f;
                };
            };
            int g;
        } h;
    } foo_t;

    foo_t foo1 = {.b = 1, 2, {3, 4}};
    expect(0, foo1.a);
    expect(1, foo1.b);
    expect(2, foo1.c);
    expect(3, foo1.h.d);
    expect(3, foo1.h.e);
    expect(4, foo1.h.f);
    expect(3, foo1.h.g);
}

int main()
{
    test1();
    test2();
    test_literal();
    test_dup();
    test_array();
    test_string();
    test_struct();
    test_primitive();
    test_nested();
    test_array_designator();
    test_struct_designator();
    test_complex_designator();
    test_zero();
    test_typedef();
    test_excessive();
    test_struct_anonymous_1();
    test_struct_anonymous_2();
    test_struct_anonymous_complex();
    return 0;
}


