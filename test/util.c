#include "test.h"

//#define expect(a, b) 

typedef struct { int a, b, c, d; } MyType;

int sum(MyType x) {
  return x.a + x.b + x.c + x.d;
}

static void test_struct() {
  MyType myType = {2, 3, 4, 5};
  expect(14, sum(myType));
}

int main() {
  test_struct();
  return 0;
}
