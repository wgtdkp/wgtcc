#include <stdio.h>

int main(void)
{
    typedef struct {
        unsigned char a: 1;
        unsigned int b: 1;
        unsigned char c: 1;
        unsigned char d: 1;
        unsigned char e: 1;
        unsigned char f: 1;
        unsigned char g: 1;
        unsigned char h: 1;
    } foo_t;

    foo_t foo = {0, 0, 0, 0, 0, 0, 0, 0};
    foo.d = 1;

    //printf("%d\n", sizeof(foo.d));
    //printf("%d\n", foo.b);
    //printf("%d\n", foo.c);
    //printf("%d\n", foo.d);
    //printf("%d\n", foo.e);
    //printf("%d\n", foo.f);
    //printf("%d\n", foo.g);
    //printf("%d\n", foo.h);


    return 0;
}
