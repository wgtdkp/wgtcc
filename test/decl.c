// Copyright 2012 Rui Ueyama. Released under the MIT license.
// @wgtcc: passed

#include "test.h"

static void t1() {
    int a = 1;
    expect(3, a + 2);
}

static void t2() {
    int a = 1;
    int b = 48 + 2;
    int c = a + b;
    expect(102, c * 2);
}

static void t3() {
    int a[] = { 55 };
    int *b = a;
    expect(55, *b);
}

static void t4() {
    int a[] = { 55, 67 };
    int *b = a + 1;
    expect(67, *b);
}

static void t5() {
    int a[] = { 20, 30, 40 };
    int *b = a + 1;
    expect(30, *b);
}

static void t6() {
    int a[] = { 20, 30, 40 };
    expect(20, *a);
}

static void decl_array() {
    int arr[4][4];
    int (*p)[4] = arr[2];
    p[0][0] = 1, p[0][1] = 2, p[0][2] = 3, p[0][3] = 4;
    p[1][0] = 5, p[1][1] = 6, p[1][2] = 7, p[1][3] = 8;
    expect(1, arr[2][0]);
    expect(2, arr[2][1]);
    expect(3, arr[2][2]);
    expect(4, arr[2][3]);
    expect(5, arr[3][0]);
    expect(6, arr[3][1]);
    expect(7, arr[3][2]);
    expect(8, arr[3][3]);
}

static int ((t7))();
static int ((*t8))();
static int ((*(**t9))(int*(), int(*), int()));

static void test_signed() {
    struct S {
        unsigned a;
        signed b;
        int c;
    };
}

static void test_signed_long() {
    unsigned long a;
    long b;
    signed long c;

    int* p = 1 ? &b: &c;
}

static void test_signed_llong() {
    unsigned long long a;
    long long b;
    signed long long c;

    int* p = 1 ? &b: &c;
}

int main() {
    t1();
    t2();
    t3();
    t4();
    t5();
    t6();
    decl_array();
    test_signed();
    test_signed_long();
    test_signed_llong();
    return 0;
}
