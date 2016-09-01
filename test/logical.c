// @wgtdkp: passed

#include "test.h"

static void test1() {
	int a = 3, b = 4;
	expect(1, a && b);
	expect(1, a || b);
	expect(0, !a || !b);
	expect(1, !a || b);
	expect(1, a || !b);
}

static void test2() {
	char a, b;
	a = 3;
	b = 4;
	expect(1, a && b);
	expect(1, a || b);
	expect(0, !a || !b);
	expect(1, !a || b);
	expect(1, a || !b);
}

static void test3() {
	float a, b;
	a = 3.0f;
	b = 4.0f;
	expect_float(1.0f, a && b);
	expect_float(1.0f, a || b);
	expect_float(0.0f, !a || !b);
	expect_float(1.0f, !a || b);
	expect_float(1.0f, a || !b);
}


int main()
{
	test1();
	test2();
	test3();
	return 0;
}