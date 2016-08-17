#include <stdio.h>

typedef void (*f_t) (void);

void test(void)
{

}

f_t f = test;

char ch;

const char* p = "abc\t";
const char* q = "ABC\t";

int main(void)
{
    f();
    return 0;
}