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


static double funcf(double f1, double f2, double f3, double f4, double f5, double f6, double f7, double f8) {
  f1 = 1;
  f2 = 2;
  f3 = 3;
  
  printf("%f\n", f1);
  printf("%f\n", f2);
  printf("%f\n", f3);
  return 0;
}

static void test() {
  funcf(1, 2, 3, 4, 5, 6, 7, 8);
  //expect(28, sumi(8, 0, 1, 2, 3, 4, 5, 6, 7));
  //expect(210, sumi(21, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20));
  //expect(2, maxi(2, 1, 2));
  //expect(44, maxi(9, -4, 6, 8, 3, 9, 11, 32, 44, 29));
  //expectf(9.0f, maxf(3, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f));
  //expectf(45.0f, sumf(9, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f));
}

int main() {
  test();
  return 0;
}
