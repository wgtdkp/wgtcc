#include <stdio.h>


typedef struct {
    int a;
    int b;
    struct {
        int c;
        int d;
        int e;
    } f;
    int g;
    int h;
} foo_t;

int a;
int b;
int* p = (&a) ? &a: 0;


int main(void)
{
/*
    foo_t arr[3] = {
        [0] = {1},
        2,
        3
    };
*/
    int a;
    static int* p = &a;
    //printf("arr[0].a:%d\n", arr[0].a);
    //printf("arr[0].b:%d\n", arr[0].b);
    //printf("arr[0].f.c:%d\n", arr[0].f.c);
    return 0;
}
