#include <stdbool.h>
#include "test.h"
#include <stdarg.h>



int main() {
  int va_start;
  bool b;
  float f = 0.5;
  f = -f;
  b = f;
  int c;
  b = c;
  printf("%d\n", b);
  return 0;
}
