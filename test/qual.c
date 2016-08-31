struct foo_t {
    int c;
};

static void test1() {
    struct foo_t f1;
    const struct foo_t f2;
    f1 = f2;
}


static void test2() {
    typedef int int_t;
    int_t a;
    const int_t b = 0;
    a = b;
}
