// @wtdkp: passed

#include "test.h"

int a = 3;
void test() {
  extern int a;
  expect(a, 3);
  {
    int a = 4;
    expect(a, 4);
  }
  {
    extern int a;
    expect(a, 3);
    a = 5;
    {
      extern int a;
      expect(a, 5);
    }
  }
}

int main() {
  test();
  return 0;
}
