#include "test.h"

void test1(void)
{
    typedef void(func_t)(void);
    func_t* func = &test1;

}

void test1(void)
{
    typedef void(func_t)(void);
    func_t func;

}

int main(void)
{
    test1();
    return 0;
}
