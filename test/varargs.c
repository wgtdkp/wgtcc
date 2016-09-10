// @wgtdkp: passed

#include "test.h"
#include <stdarg.h>

static int sumi(int a, ...) {
  va_list args;
  va_start(args, a);
  int acc = 0;
  for (int i = 0; i < a; ++i)
    acc += va_arg(args, int);
  va_end(args);
  return acc;
}

static int maxi(int n, ...) {
  va_list args;
  va_start(args, n);
  int max = INT_MIN;
  for (int i = 0; i < n; ++i) {
    int x = va_arg(args, int);
    if (x > max) max = x;
  }
  va_end(args);
  return max;
}

static float maxf(int n, ...) {
  va_list args;
  va_start(args, n);
  float max = 0.0f;
  for (int i = 0; i < n; ++i) {
    float x = va_arg(args, double);
    if (x > max) max = x;
  }
  va_end(args);
  return max;
}

static float sumf(int a, ...) {
  va_list args;
  va_start(args, a);
  float acc = 0;
  for (int i = 0; i < a; ++i) {
    acc += va_arg(args, double);
  }
  va_end(args);
  return acc;
}

static double sumif(int n, ...) {
  va_list args;
  va_start(args, n);
  double acc = 0;
  for (int i = 0; i + 1 < n; i += 2) {
    acc += va_arg(args, int);
    acc += va_arg(args, double);
  }
  va_end(args);
  return acc;
}

typedef struct {
  int x;
  int y;
} pos_t;

static int test_struct(int a1, int a2, int a3, int a4, int a5, int a6, int a7, ...) {
  va_list args;
  va_start(args, a7);
  pos_t pos = va_arg(args, pos_t);
  return pos.x + pos.y;
}

static void test() {
  expect(28, sumi(8, 0, 1, 2, 3, 4, 5, 6, 7));
  expect(210, sumi(21, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20));
  expect(2, maxi(2, 1, 2));
  expect(44, maxi(9, -4, 6, 8, 3, 9, 11, 32, 44, 29));
  expectf(9.0f, maxf(9, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f));
  expectf(45.0f, sumf(9, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f));
  expect(210.0f, sumif(20, 1, 2.0f, 3, 4.0f, 5, 6.0f, 7, 8.0f, 9, 10.0f, 11, 12.0f, 13, 14.0f, 15, 16.0f, 17, 18.0f, 19, 20.0f));

  pos_t pos = {3, 7};
  expect(10, test_struct(1, 2, 3, 4, 5, 6, 7, pos));
}

int main() {
  test();
  return 0;
}
