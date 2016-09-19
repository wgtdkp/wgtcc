#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  int size;
  int width;
  int capacity;
  void* data;
} vec_t;

#define VEC(type) (vec_t) { \
  .size=0,                  \
  .width=sizeof(type),      \
  .capacity=0,              \
  .data=NULL                \
}

void vec_push(vec_t* vec, void* data);
void vec_pop(vec_t* vec);
void vec_clear(vec_t* vec);
void* vec_at(vec_t* vec, int idx);
void* vec_back(vec_t* vec);
bool vec_empty(const vec_t* vec);
vec_t vec_copy(vec_t* src);

#endif
