#include <stdio.h>


static void test1()
{
    int a;
    //a++ = 3;
    //int* c = &(a = 3);
}

static void test2()
{
    char a; 
    int* b;
    b = &((int)a);

}

int main()
{
    test1();
    test2();
    return 0;
}
