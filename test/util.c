#include "test.h"

typedef struct {
    int line;
    int column;
} Pos;

static Pos get_pos() {
  return (Pos){1, 2};
}

static void test() {
  Pos pos = get_pos();
  expect(1, pos.line);
  expect(2, pos.column);
}

int main() {
  test();
  return 0;
}