#define UINT_MAX 4294967295U

#define MAX(a, b) a + a

int a = MAX(UINT_MAX, UINT_MAX);

int b = UINT_MAX;

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