#include <stdio.h>

int main(int argc, char** argv)
{
    int a, b, c, d, e, f, g, h, i, j, k;
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

    //a = (a + (b + (c + (d + (e + (f + (g + (h + (i + j)))))))));
    a = (a + b) + (c + d);
    
    printf("%d\n", a);

    return 0;
}