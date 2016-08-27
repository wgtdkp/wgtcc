struct foo_t {
    int c;
};

static void test(void)
{
    struct foo_t f1;
    const struct foo_t f2;
    f1 = f2;
}
