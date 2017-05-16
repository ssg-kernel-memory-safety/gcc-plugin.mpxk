#include "test.h"
#include "assert_tests.h"

int main(int argc, char **argv)
{
	int fails;

	/* Make sure our tests, mocks, etc are correct */
	if (! assert_test_stuff_okay()) {
		fprintf(stderr, "Tests don't work as expected, quitting\n");
		return -1;
	}

	fails += !test_dumps();
	fails += !test_va_args();
	fails += !test_func_args();
	fails += !test_complex_func();

	return (fails ? -1 : 0);
}
