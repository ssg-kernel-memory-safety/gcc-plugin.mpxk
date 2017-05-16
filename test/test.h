#ifndef _MPXK_TEST_H_
#define _MPXK_TEST_H_

#include <stdio.h>

int test_va_arg(void);

#define test __attribute__((noinline))
#define test_legacy __attribute__((noinline)) __attribute__((bnd_legacy))

__attribute__((always_inline)) __attribute__((bnd_legacy))
inline static int __assert_bnds(unsigned long e, unsigned long w)
{
	if (e == w)
		return 1;
	printf("bounds assertion failed, expected %lu, was %lu\n", e, w);
	return 0;
}

#define assert_bnds(p, e)  __assert_bnds((e), 1 + 		\
		((unsigned long) __bnd_get_ptr_ubound(p) -  	\
		(unsigned long) __bnd_get_ptr_lbound(p)))

#define fail_msg(...) do {		\
	fprintf(stderr, __VA_ARGS__);	\
	return 0;			\
} while (0)

#define test_ok(name) do {		\
	printf("%s ok\n", name);	\
	return 1;			\
} while (0)

#define test_fail(name) do {		\
	printf("%s FAILED!\n", name);	\
	return 0;			\
} while (0)

#define test_res(name) do { 		\
	if (res)			\
		test_ok(__func__);	\
	test_fail(__func__);		\
} while (0)

#endif /* _MPXK_TEST_H_ */
