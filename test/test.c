/*
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 */
#include "test.h"

int main(int argc, char **argv)
{
	int fails;

	/* Make sure our tests, mocks, etc are correct */
	if (! test_the_tests()) {
		fprintf(stderr, "Tests don't work as expected, quitting\n");
		return -1;
	}

	fails += !test_dumps();
	fails += !test_va_args();
	fails += !test_func_args();
	fails += !test_complex_func();

	return (fails ? -1 : 0);
}

int __assert_bnds(const void *p, unsigned long e,
		const char *file, const int linenr, const char *func)
{
	unsigned long w;

	w = 1ul + (unsigned long) __bnd_get_ptr_ubound(p) -
		  (unsigned long) __bnd_get_ptr_lbound(p);
	if (e == w)
		return 1;
	fprintf(stderr, "%s:%d:%s: bounds assertion failed, expected %lu, was %lu\n",
			file, linenr, func, e, w);
	return 0;
}

