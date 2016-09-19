#include "test.h"

static void test_bitfield_mix() {
  typedef union {
    char b: 4;
    char a;    
    char c: 3;
    int d: 6;
  } foo_t;
  expect(4, sizeof(foo_t));
}

int main() {
  test_bitfield_mix();
  return 0;
}
