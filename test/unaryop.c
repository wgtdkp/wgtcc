// @wgtdkp: passed

#include "test.h"

static void test_inc_dec() {
  
  int a = 3, b = 4;
  expect(3, a++);
  expect(4, a);
  expect(3, --a);
  expect(5, --a + --b);
  expect(2, a--);
  expect(1, a);
}


static void test_inc_dec_float() {
  float a = 3.0f, b = 4.0f;
  expect_float(3.0f, a++);
  expect_float(4.0f, a);
  expect_float(3.0f, --a);
  expect_float(5.0f, --a + --b);
  expect_float(2.0f, a--);
  expect_float(1.0f, a);
}

static void test_inc_dec_pointer() {
  int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int* p = arr;
  expect(0, *p++);
  expect(1, *p);
  expect(2, *++p);
}

static void test_minus_plus() {
  int a = 3, b = 4;
  expect(-3, -a);
  expect(-3, +-a);
  expect(3, -(-a));
  expect(0, -0);
  expect(-1, a - b);
  expect(0xffffffff, -1);
}


int main() {
  test_inc_dec();
  test_inc_dec_float();
  test_inc_dec_pointer();
  test_minus_plus();
  return 0;
}
