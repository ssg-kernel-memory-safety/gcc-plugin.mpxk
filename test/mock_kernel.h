/*
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 */
#ifndef _MOCK_KERNEL_H_
#define _MOCK_KERNEL_H_

#include <stddef.h>

#define GFP_KERNEL 0

typedef int gfp_t;

void *kmalloc(size_t size, gfp_t flags);
void kfree(void *ptr);
size_t ksize(void *ptr);
int PageSlab_virt_to_page(void *ptr);

#endif /* _MOCK_KERNEL_H_ */
