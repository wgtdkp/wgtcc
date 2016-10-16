#include <stdio.h>

typedef struct {
  int a;
  int b;
} foo_t;

foo_t* foo = &(foo_t) {3, 4};

int main() {
  printf("%d, %d\n", foo->a, foo->b);
  return 0;
}
