#include <stdio.h>


static void test1(void)
{
    int a;
    //a++ = 3;
    //int* c = &(a = 3);
}

static void test2(void)
{
    char a; 
    int* b;
    b = &((int)a);

}

int main(void)
{
    test1();
    test2();
    return 0;
}
