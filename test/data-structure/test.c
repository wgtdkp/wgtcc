#include "vector.c"

#include <stdio.h>

int main() {
  vec_t vec = VEC(int);
  for (int i = 0; i < 5; ++i)
    vec_push(&vec, &i);

  for (int i = 0; i < vec.size; ++i)
    printf("%d ", *(int*)vec_at(&vec, i));
  printf("\n");

  int size = vec.size;
  for (int i = 0; i < size; ++i) {
    int ele = *(int*)vec_back(&vec);
    printf("%d ", ele);
    vec_pop(&vec);
  }
  printf("\n");
  return 0;
}
