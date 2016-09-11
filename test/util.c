#include "test.h"
#include "add.h"

static void test() {
  int i = MAX;
  expect(3, i);
  expect(3, ADD(1, 2));
}

int main() {
  test();
  return 0;
}
