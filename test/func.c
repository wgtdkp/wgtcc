#include <stdio.h>

typedef void(*func_t)(void);

static void func(void)
{
    printf("func\n");
}

static void func1(void)
{
    printf("func1\n");
}

int main(void)
{
    func_t f = func;
    f();
    f = func1;
    f();
    return 0;
}

