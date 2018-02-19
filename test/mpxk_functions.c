/*
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 */
#include "mock_kernel.h"
#include <assert.h>

#define wrap_wrapper __attribute__((bnd_legacy)) __attribute__((always_inline)) \
static inline

wrap_wrapper
void *__mpxk_wrapper_kmalloc(size_t s, gfp_t f)
{
	return kmalloc(s, f);
}

void *mpxk_wrapper_kmalloc(size_t s, gfp_t f)
{
	void *p = __mpxk_wrapper_kmalloc(s, f);

	if (p)
		return __bnd_set_ptr_bounds(p, s);
	return __bnd_null_ptr_bounds(p);
}

void *mpxk_load_bounds(void *ptr)
{
	size_t size;

	if (PageSlab_virt_to_page(ptr)) {
		size = ksize(ptr);
		if (size == 0)
			return __bnd_null_ptr_bounds(ptr);
		return __bnd_set_ptr_bounds(ptr, size);
	}

	assert(0 && "Assume our tests never hit this!!!");

	return __bnd_init_ptr_bounds(ptr);
}
