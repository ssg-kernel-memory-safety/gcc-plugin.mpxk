#include "test.h"
#include "assert_tests.h"
#include "test_dump.h"

int main(int argc, char **argv)
{
	/* Make sure our tests, mocks, etc are correct */
	if (! assert_test_stuff_okay()) {
		fprintf(stderr, "Tests don't work as expected, quitting\n");
		return -1;
	}

	return (! run_test_dumps());
}
