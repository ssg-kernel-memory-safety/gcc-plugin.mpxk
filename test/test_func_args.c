/* Test function bounds parameter propagation, there seems to be some cases where argument bounds
 * get passed via bndstx/bndldx even if registers were available. Namely, it seems that arguments
 * beyond the 6th sometimes have their bounds passed via registers regardless of free bnd
 * registers. This could also be an artefact of something completely different... */

#include "test.h"
#include "mock_kernel.h"

#define PTR_SIZE 634

test
static int the_sixth(int i1, int i2, int i3, int i4, int i5, void *ptr)
{
	assert_bnds(ptr, PTR_SIZE);
	return i1 + i2 + i3 + i4 + i5;
}

test
static int the_sixth_and_beyond(int i1, int i2, int i3, int i4, int i5, int i6, void *ptr)
{
	assert_bnds(ptr, PTR_SIZE);
	return i1 + i2 + i3 + i4 + i5 + i6;
}

test
static int non_void_sixth(int i1, int i2, int i3, int i4, int i5, int i6, char *ptr)
{
	assert_bnds(ptr, PTR_SIZE);
	return i1 + i2 + i3 + i4 + i5 + i6;
}

test
int test_func_args(void)
{
	printf("%s", __func__);
	int fails = 0;
	void *ptr;
	char *c_ptr;

	/* Test the case from net/core/dev.c */
	ptr = kmalloc(PTR_SIZE, GFP_KERNEL);

	c_ptr = ptr;

	fails += !the_sixth(1, 2, 3, 4, 5, ptr);
	fails += !the_sixth_and_beyond(1,2,3,4,5,6, ptr);
	fails += !non_void_sixth(1,2,3,4,5,6, c_ptr);

	kfree(ptr);

	printf(" %s\n", (fails ? "FAILED" : "ok"));
	return !fails;
}
