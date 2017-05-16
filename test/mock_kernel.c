#include "mock_kernel.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define CACHE_SIZE 100
static void *ptrs[CACHE_SIZE] = { NULL };
static size_t ptr_sizes[CACHE_SIZE];

__attribute__((bnd_legacy))
static void save_bounds(void *ptr, size_t size)
{
	int i;
	for (i = 0; i < CACHE_SIZE; i++) {
		if (ptrs[i] == NULL) {
			ptrs[i] = ptr;
			ptr_sizes[i] = size;
			return;
		}
	}
	assert(0 && "Make sure we never fail due to cache size");
}


__attribute__((bnd_legacy))
void *kmalloc(size_t size, gfp_t flags)
{
	(void) flags;
	void *ptr = malloc(size);
	save_bounds(ptr, size);
	return ptr;
}

__attribute__((bnd_legacy))
void kfree(void *ptr)
{
	int i;
	for (i = 0; i < CACHE_SIZE; i++) {
		if (ptrs[i] == ptr) {
			ptrs[i] = NULL;
			ptr_sizes[i] = 0;
			break;
		}
	}

	free(ptr);
}


/* Horrible stand-in for PageSlab(virt_to_page(ptr)) */
__attribute__((bnd_legacy))
int PageSlab_virt_to_page(void *ptr)
{

	int i;
	for (i = 0; i < CACHE_SIZE; i++) {
		if (ptrs[i] == ptr)
			return 1;
	}
	return 0;
}

__attribute__((bnd_legacy))
size_t ksize(void *ptr)
{
	int i;
	for (i = 0; i < CACHE_SIZE; i++) {
		if (ptrs[i] == ptr)
			break;
	}

	if (i != CACHE_SIZE) {
		return ptr_sizes[i];
	}

	assert(!"Plan on never getting here for these tests!");
}
