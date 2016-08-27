
struct foo_t {
    int c;
};

struct bar_t {
    int c;
};

int main(void)
{
    struct foo_t f1;
    struct bar_t b2 = f1;
    return 0;
}