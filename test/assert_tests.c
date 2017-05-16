#include "assert_tests.h"
#include "mock_kernel.h"
#include "test.h"
#include "assert.h"

test_legacy
int test_kmalloc_kszie()
{
	void *ptr = kmalloc(40, GFP_KERNEL);
	size_t s = ksize(ptr);
	kfree(ptr);

	if (!(s >= 40 && s <= 400))
		fail_msg("Expected size %lu, was %lu\n", 40LU, s);

	return 1;
}

test int assert_test_stuff_okay(void)
{
	return test_kmalloc_kszie();
}
