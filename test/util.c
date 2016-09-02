#include "test.h"

static int add(int, int);

static int add(int a, int b) {
    return a + b;
}

int main() {
    expect(3, add(1, 2));
    //int c = add(1, 2);
    return 0;
}


/*
typedef struct {
  int a;
  int b;
} Type;
static Type* make_ptr_type(Type *ty);
static Type *make_type(Type *tmpl) {
    Type *r;// = malloc(sizeof(Type));
    *r = *tmpl;
    return r;
}

static Type* make_ptr_type(Type *ty) {
    return make_type(&(Type){ 8 });
}
*/