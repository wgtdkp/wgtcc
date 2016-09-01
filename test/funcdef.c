#include "test.h"

void test1()
{
    typedef void(func_t)();
    func_t* func = &test1;

}

void test1()
{
    typedef void(func_t)();
    func_t func;

}

int main()
{
    test1();
    return 0;
}
