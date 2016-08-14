struct foo {
    char c;
    short s;
    struct bar {
        int i;
        long l;
        double d;
    } b;
    float f;
};

int main(void)
{
    struct foo f = {1, 1, {1, 1, 1}, 1};
    return 0;
}
