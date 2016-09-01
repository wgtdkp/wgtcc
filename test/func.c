#include <stdio.h>

typedef void(*func_t)();

static void func()
{
    printf("func\n");
}

static void func1()
{
    printf("func1\n");
}

static void test1()
{
    func_t f = func;
    func_t* pf = &f;
    (*pf)();
}

static void test_param(int arr[])
{

}

int main()
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
