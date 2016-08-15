#include <stdio.h>

typedef struct {
    char a;
    char b;
    struct {
        char d;
        char e;
    } ss;
    char c;
} foo_t;

int main(void)
{
    //printf("%ld \n", *(long*)&f);
    //int c = {10, 11, 12};
    foo_t foo = {1, 2, 3, .c = 5, 5};
    return 0;
}
