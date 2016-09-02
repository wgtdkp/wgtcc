static void test() {
    float a = 3.0f;
    float b;
    b = a++;
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