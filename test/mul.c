// @wgtdkp: passed

#include "test.h"

static void test_int() {
	int s1, s2;
	s1 = -3;
	s2 = -4;

	expect(-12, s1 * 4);
	expect(12, s1 * -4);
	expect(12, s1 * s2);
	expect(2, s2 / -2);
	expect(1, s1 / -2);
	expect(0, s1 / s2);
	expect(-3, s1 % s2);
	expect(0, s2 % s2);
	expect(1, 1 % s2);
}

static void test_uint() {
	unsigned s1, s2;
	s1 = 3U;
	s2 = 4U;
	expect(12U, s1 * 4U);
	expect((unsigned)(-16U), s2 * -4U);
	expect(12U, s1 * s2);
	expect(2U, s2 / 2U);
	expect(1U, s1 / 2U);
	expect(0U, s1 / s2);
	expect(3U, s1 % s2);
	expect(0, s2 % s2);
	expect(1U, 1U % s2);
}

static void test_long() {
	long s1, s2;
	s1 = -3L;
	s2 = -4L;
	expect(-12L, s1 * 4L);
	expect(12L, s1 * -4L);
	expect(12L, s1 * s2);
	expect(2L, s2 / -2L);
	expect(1L, s1 / -2L);
	expect(0L, s1 / s2);
	expect(-3L, s1 % s2);
	expect(0L, s2 % s2);
	expect(1L, 1L % s2);
}


static void test_ulong() {	
	unsigned  long s1, s2;
	s1 = 3UL;
	s2 = 4UL;
	expect(12UL, s1 * 4UL);
	expect((unsigned long)(-16UL), s2 * -4UL);
	expect(12UL, s1 * s2);
	expect(2UL, s2 / 2UL);
	expect(1UL, s1 / 2UL);
	expect(0UL, s1 / s2);
	expect(3UL, s1 % s2);
	expect(0UL, s2 % s2);
	expect(1UL, 1UL % s2);
}


int main(void) {
	test_int();
	test_uint();
	test_long();
	test_ulong();
	return 0;
}


