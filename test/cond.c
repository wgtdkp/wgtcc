// @wgtdkp: passed

#include "test.h"

void test1()
{
    int i = 4;
    if (i == 0) {
        expect(0, 1);
    } else if (i == 1) {
        expect(0, 1);
    } else if (i == 2) {
        expect(0, 1);
    } else if (i == 3) {
        expect(0, 1);
    } else if (i == 4) {
        expect(1, 1);
    } else {
        expect(0, 1);
    }

}

void test2() {
    int i = 7;
    switch (i) {
    case 1: expect(0, 1); break;
    case 2: expect(0, 1); break;
    case 3: expect(0, 1); break;
    case 4: expect(0, 1); break;
    case 5: expect(0, 1); break;
    case 6: expect(0, 1); break;
    case 7: expect(1, 1); break;
    default: expect(0, 1); break;
    }
}

void test3()
{
    int i = 3;
    int j = i == 0 ? -1: (i == 1 ? -2: (i == 2 ? -3: (i == 3 ? -4: -5)));
    expect(j, -4);
}

int main()
{
    test1();
    test2();
    test3();
    return 0;
}
