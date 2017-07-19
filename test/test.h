#ifndef _MPXK_TEST_H_
#define _MPXK_TEST_H_

#include <stdio.h>

int test_the_tests(void);
int test_dumps(void);
int test_va_args(void);
int test_func_args(void);
int test_complex_func(void);

#define test __attribute__((noinline))
#define test_legacy __attribute__((noinline)) __attribute__((bnd_legacy))

#define assert_bnds(p, e)  __assert_bnds((p), (e), __FILE__, __LINE__, __func__)
int __assert_bnds(const void *p, unsigned long e,
		const char *file, const int linenr, const char *func);

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
