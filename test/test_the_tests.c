/*
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 */
#include "mock_kernel.h"
#include "test.h"

static test_legacy
int test_kmalloc_kszie()
{
	void *ptr = kmalloc(40, GFP_KERNEL);
	size_t s = ksize(ptr);
	kfree(ptr);

	if (!(s >= 40 && s <= 400))
		fail_msg("Expected size %lu, was %lu\n", 40LU, s);

	return 1;
}

test
int test_the_tests(void)
{
	return test_kmalloc_kszie();
}
