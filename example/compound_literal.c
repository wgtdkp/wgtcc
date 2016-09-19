#include <stdio.h>
#include <stdlib.h>

#define CAST(s_t, d_t, sv)  \
  (union {s_t sv; d_t dv;}){sv}.dv

int main() {
  float f = 4.5;
  int i = CAST(float, int, f);
  printf("%d\n", i);
  return 0;
}
