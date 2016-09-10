#include "test.h"

static void test() {
  expect(6, ((struct { int x[3]; }){ 5, 6, 7 }.x[1]));
}

int main() {
  test();
  return 0;
}
