struct foo {
    int a;
    struct bar {
        int c;
    } b;
} f;

int test(int aa, int bb, int cc, int dd, int ee)
{
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n;
    a = 1;
    b = 2;
    c = 3;
    d = 4;
    e = 5;
    f = 6;
    g = 7;
    h = 8;
    i = 9;
    j = 10;
    k = 11;
    l = 12;
    m = 13;
    n = 14;
    n = (a + b) + ((c + d) + ((e + f) + ((g + h) + (i + j))));
    return 0;
}
