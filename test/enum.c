// @wgtdkp: passed

#include "test.h"


void test()
{
	typedef enum {
		AA,
		BB = 3,
		CC = 3,
		DD,
	} idtype_t;

	expect(1, AA);
	expect(3, BB);
	expect(4, DD);
	{
		int AA = -4;
		int BB = -5;
		expect(-4, AA);
		expect(-5, BB);
	}
}

int main()
{
    test();
    return 0;
}
