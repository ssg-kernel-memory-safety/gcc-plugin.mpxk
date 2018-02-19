/*
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 *
 * Test for issue with how va_arg functions are handled. Is at presnet a
 * problem in various kernel functions
 */

#include "test.h"
#include "mock_kernel.h"
#include <stdarg.h>

#define KERN_EMERG "emerg"
#define DEVICE_SIZE 23

struct va_format {
	const char *fmt;
	va_list *va;
};

test
static int __netdev_printk(const char *level,
		const void *dev, struct va_format *vaf)
{
	assert_bnds(dev, (DEVICE_SIZE));
	assert_bnds(vaf, sizeof(struct va_format));
	return 1;
}


test
/* Copy of netdev_emerg from net/core/dev.c (except for return value) */
static int netdev_emerg(const void *dev, const char *fmt, ...)
{
	int r;
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	r = __netdev_printk(KERN_EMERG, dev, &vaf);

	va_end(args);

	return r;
}

test
static int basic_va_case(int count, ...)
{
	int i, s = 0, check_count = 0;
	va_list vl;
	void *ptr;

	va_start(vl, count);

	for (i = 0; i < count; i++) {
		ptr = va_arg(vl, void *) ;
		assert_bnds(ptr, (DEVICE_SIZE * (s+1)));
		check_count++;
		s++;
		s = s%10;
	}

	va_end(vl);

	if (check_count != count)
		fprintf(stderr, "%s: Expected %d checks, but only %d executed\n",
				__func__, count, check_count);

	return (check_count == count);
}

test
int test_va_args(void)
{
	int fails = 0;
	void *dev;
	void *p1, *p2, *p3, *p4, *p5,
	     *pa1, *pa2, *pa3, *pa4, *pa5;

	printf("%s", __func__);

	/* Test the case from net/core/dev.c */
	dev = kmalloc(DEVICE_SIZE, GFP_KERNEL);
	fails += !netdev_emerg(dev, "Does this fail alreay?\n");
	kfree(dev);

	/* Test lotsa va_args */
	p1 = kmalloc(DEVICE_SIZE * 1, GFP_KERNEL);
	p2 = kmalloc(DEVICE_SIZE * 2, GFP_KERNEL);
	p3 = kmalloc(DEVICE_SIZE * 3, GFP_KERNEL);
	p4 = kmalloc(DEVICE_SIZE * 4, GFP_KERNEL);
	p5 = kmalloc(DEVICE_SIZE * 5, GFP_KERNEL);
	pa1 = kmalloc(DEVICE_SIZE * 6, GFP_KERNEL);
	pa2 = kmalloc(DEVICE_SIZE * 7, GFP_KERNEL);
	pa3 = kmalloc(DEVICE_SIZE * 8, GFP_KERNEL);
	pa4 = kmalloc(DEVICE_SIZE * 9, GFP_KERNEL);
	pa5 = kmalloc(DEVICE_SIZE * 10, GFP_KERNEL);

	fails += !basic_va_case(1, p1);
	fails += !basic_va_case(2, p1, p2);
	fails += !basic_va_case(3, p1, p2, p3);
	fails += !basic_va_case(4, p1, p2, p3, p4);
	fails += !basic_va_case(5, p1, p2, p3, p4, p5);
	fails += !basic_va_case(10, p1, p2, p3, p4, p5, pa1, pa2, pa3, pa4, pa5);
	fails += !basic_va_case(20, p1, p2, p3, p4, p5, pa1, pa2, pa3, pa4, pa5,
			p1, p2, p3, p4, p5, pa1, pa2, pa3, pa4, pa5);

	printf(" %s\n", (fails ? "FAILED" : "ok"));
	return !fails;
}
