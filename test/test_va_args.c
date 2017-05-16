/* Test for issue with how va_arg functions are handled.
 * Is at presnet a problem in various kernel functions */
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
	int i, check_count = 0;
	va_list vl;
	void *ptr;

	va_start(vl, count);

	for (i = 0; i < 5; i++) {
		ptr = va_arg(vl, void *) ;
		assert_bnds(ptr, (DEVICE_SIZE * (i+1)));
		check_count++;
	}

	va_end(vl);

	return (check_count == count);
}

test
int test_va_args(void)
{
	int fails = 0;
	void *dev;
	void *p1, *p2, *p3, *p4, *p5;

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

	fails += !basic_va_case(5, p1, p2, p3, p4, p5);

	printf(" %s\n", (fails ? "FAILED" : "ok"));
	return !fails;
}
