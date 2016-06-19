//#include <stdio.h>
/*
union A {
    int a;
    float b;
};
*/

int a = 1;
int b = a;
int main(void)
{
    //union A a1 = { 3 };

    int arr[1]/* = {
        [0] = 1, 
        [2] = 3,
        //arr[0],
    }*/;
/*
    for (int i = 0; i < 5; i++) {
        printf("%d\n", arr[i]);
    }
*/
    //union A a1;
    int c = arr ? 3: 4;
    return 0;
}