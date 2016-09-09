//#include "test.h"


static void test() {
  union {
    _Alignas(8) char a;
    int b;
  };
}

int main() {
  test();
  return 0;
}