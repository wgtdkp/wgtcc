// @wgtdkp: passed

#include "test.h"


static void test_shift() {
	int a = 3, b = 4;
	expect(48, 3 << 4);
	expect(0, 3 >> 4);
	expect(3, a << 0);
	expect(4, b >> 0);
}

static void test_or() {
	int a = 3, b = 4;
	expect(7, a | b);
	expect(11, a | (b << 1));
	expect(3, a | a);
	expect(4, b | b);
}

static void test_xor() {
	int a = 3, b = 4;
	expect(0, a ^ a);
	expect(7, a ^ b);
	expect(3, a ^ 0);
	expect(-3, (a ^ -1) + 1);
}

static void test_and() {
	int a = 3, b = 4;
	expect(0, a & b);
	expect(3, a & a);
	expect(0, a & 0);
	expect(4, b & 7);
}

static void test_not() {
	int a = 3, b = 4;
	expect(-3, (unsigned)~a + 1);
	expect(-1, ~0);
}

int main()
{
	test_shift();
	test_or();
	test_xor();
	test_and();
	test_not();
	return 0;
}
