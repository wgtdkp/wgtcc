#include <stdio.h>

/*
void test(int a1, float f1, float f2, float f3, 
        int a2, float f4, float f5, int a3, float f6, 
        float f7, int a4, float f8, int a5, float f9, int a6, float f10, int a7, int a8)
{

}

int main(void)
{
    test(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18);
    return 0;
}
*/

int test(int a)
{
    if (a == 1)
        return 1;
    else
        return 10;
}

int main(void)
{
    float d = 5.3;
    //printf("hello world\n");
    printf("test: %d\n", test(1));
    printf("test: %d\n", test(2));
    printf("test: %d\n", test(3));
    
    return 0;
}
