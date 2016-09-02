#define stringify(x) #x
#define identity(x) stringify(x)
#define m15(x) x x

int main() {
  printf("%s\n", identity(m15(a)));
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