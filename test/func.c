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

static void test1(void)
{
    func_t f = func;
    func_t* pf = &f;
    (*pf)();
}

static void test_param(int arr[])
{

}

int main(void)
{
    func_t f = func;
    f();
    f = func1;
    f();

    test1();
    int arr[3];    
    test_param(arr);
    return 0;
}
