#include "vector.h"

#include <assert.h>
#include <memory.h>

void vec_push(vec_t* vec, void* data) {
  if (vec->size >= vec->capacity) {
    vec->capacity = vec->capacity * 2 + 1;
    void* new_data = malloc(vec->width * vec->capacity);
    memcpy(new_data, vec->data, vec->width * vec->size);
    free(vec->data);
    vec->data = new_data;
  }
  memcpy(vec->data + vec->width * vec->size, data, vec->width);
  ++vec->size;
}

void vec_pop(vec_t* vec) {
  assert(vec->size);
  --vec->size;
}

void vec_clear(vec_t* vec) {
  free(vec->data);
  vec->data = NULL;
  vec->size = vec->capacity = 0;
}

void* vec_at(vec_t* vec, int idx) {
  assert(idx >= 0 && idx <= vec->size);
  return vec->data + vec->width * idx;
}

void* vec_back(vec_t* vec) {
  assert(vec->size);
  return vec_at(vec, vec->size - 1);
}

bool vec_empty(const vec_t* vec) {
  return !vec->size;
}

vec_t vec_copy(vec_t* src) {
  vec_t des = *src;
  memcpy(des.data, src->data, src->width * src->size);
  return des;
}
