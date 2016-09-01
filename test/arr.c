#include "test.h"

static void test1()
{
    int arr[3][4];
    expect(48, sizeof(arr));
    expect(16, sizeof(arr[0]));
    expect(16, sizeof(arr[1]));
    expect(16, sizeof(arr[2]));
    expect(4, sizeof(arr[2][3]));
    expect(8, sizeof(arr + 1));
    expect(16, sizeof(*(arr + 1)));
    expect(12, &arr[3][4] - arr);
}



int main()
{
    test1();
    return 0;
}
