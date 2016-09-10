#include "test.h"


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
int main() {
  pos_t pos = {3, 7};
  expect(10, test_struct(1, 2, 3, 4, 5, 6, 7, pos));
  return 0;
}
