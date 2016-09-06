#include "test.h"
#include <stdarg.h>

static int max(int a, int b) {
  static int i;
  a = i;
}

static int sum(int a, ...) {
  va_list args;
  va_start(args, a);
  printf("%p\n", va_arg(args, int));
  printf("%p\n", va_arg(args, int));
  printf("%p\n", va_arg(args, int));
  printf("%p\n", va_arg(args, int));
  printf("%p\n", va_arg(args, int));
  printf("%p\n", va_arg(args, int));
  printf("%p\n", va_arg(args, int));
  printf("%p\n", va_arg(args, int));

  va_end(args);
  return 0;
}

typedef struct {
  int a;
  int b;
} foo_t;

static int func1(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) {
  return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

static void test_func1() {
  expect(45, func1(1, 2, 3, 4, 5, 6, 7, 8, 9));
}

static void func(void* foo) {

}

static void test_func() {
  foo_t foo[1] = {{1, 2}};
  expect(1, foo[0].a);
  expect(2, foo[0].b);
  func(&foo[0]);
}

static void test() {
  sum(6, 0, 1, 2, 3, 4, 5, 6, 7);
}

int main() {
  test();
  //test_func();
  test_func1();
  return 0;
}
