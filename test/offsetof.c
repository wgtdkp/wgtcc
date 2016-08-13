#include <stdio.h>
#include <stddef.h>

typedef struct {
    int a;
    int b;
    int c;
} foo_t;

int d = offsetof(foo_t, c);

int* p = (int*)1;
int dd = d;
