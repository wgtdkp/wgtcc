// @wgtdkp: passed

#include "test.h"

static void test_char() {
  char c1, c2;
  c1 = 1;
  c2 = 2;
  expect(3, c1 + c2);
  expect(1, c2 - c1);
  expect(-1, c1 - c2);
}

static void test_uchar() {
  unsigned char c1, c2;
  c1 = 1;
  c2 = 2;
  expect(3, c1 + c2);
  expect(1, c2 - c1);
  // unsigned char is promoted to int
  expect(-1, c1 - c2);
}

static void test_short() {
  short s1, s2;
  s1 = 1;
  s2 = 2;

  expect(3, s1 + s2);
  expect(1, s2 - s1);
  expect(-1, s1 - s2);
}

static void test_ushort() {
  unsigned short s1, s2;
  s1 = 1;
  s2 = 2;

  expect(3, s1 + s2);
  expect(1, s2 - s1);
  expect(-1, s1 - s2);
}

static void test_int() {
  int s1, s2;
  s1 = 1;
  s2 = 2;

  expect(3, s1 + s2);
  expect(1, s2 - s1);
  expect(-1, s1 - s2);
}

static void test_uint() {
  unsigned int s1, s2;
  s1 = 1;
  s2 = 2;

  expect(3, s1 + s2);
  expect(1, s2 - s1);
  expect(UINT_MAX, s1 - s2);
}

static void test_long() {
  long s1, s2;
  s1 = 1;
  s2 = 2;

  expect(3, s1 + s2);
  expect(1, s2 - s1);
  expect(-1, s1 - s2);
}

static void test_ulong() {
  unsigned long s1, s2;
  s1 = 1;
  s2 = 2;

  expect(3, s1 + s2);
  expect(1, s2 - s1);
  expect(ULONG_MAX, s1 - s2);
}

static void test_float() {
  float s1, s2;
  s1 = 1.0f;
  s2 = 2.0f;

  expect(3.0f, s1 + s2);
  expect(1.0f, s2 - s1);
  expect(-1.0f, s1 - s2);  
}

static void test_double() {
  float s1, s2;
  s1 = 1.0;
  s2 = 2.0;

  expect(3.0, s1 + s2);
  expect(1.0, s2 - s1);
  expect(-1.0, s1 - s2);  
}

int main() {
  test_char();
  test_uchar();
  test_short();
  test_ushort();
  test_int();
  test_uint();
  test_long();
  test_ulong();
  test_float();
  test_double();
  return 0;
}
