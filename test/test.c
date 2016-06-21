//#include <stdio.h>
/*
union A {
    int a;
    float b;
};
*/

// Declaration
static int a, b, c = 2;
//static void d;
int arr[2][3];

typedef void(*f)(int, int);
typedef const int(*f1)(f);

static void test_if(void)
{
    int a = 3;
    if (a)
        ;

    if (a)
        a = 3;
    
    if (a) {}

    if (a) {

    } else
        ;
    
    if (a) {

    } else if (!a) {

    }
}

static void test_switch(void)
{

}

int main(void)
{

    int c = sizeof(test_if);
    return 0;
}