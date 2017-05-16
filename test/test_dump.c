/*
 * This is a horrilb dump site for (potentially temporary) LKDTM tests that are meant for in-kernel
 * runtime testing. Allows running the same tests in a stand-alone fashion outside the kernel.
 */

#include "test.h"
#include "mock_kernel.h"

/* Track test failures here, since the lkdtm function return void */
static int failed = 0;

#define fail_dump(...) do { 	\
	fprintf(stderr, __VA_ARGS__); 	\
	failed = 1; 			\
} while (0)

/* Some macros to allow verbatim copy-paste of LDTM test implementations */

typedef int bool;

#define noinline __attribute__((noinline))
#define false 0
#define true 1


#define pr_info(...)
#define verify_bounds_five_extern(...)

#define MPXK_VERIFY_BOUNDS(ptr, expected, exact) do {						\
	unsigned long range = __bnd_get_ptr_ubound(ptr) - __bnd_get_ptr_lbound(ptr) + 1;	\
	if (exact)										\
		if (range != expected)								\
			fail_dump("%s: BAD_BOUNDS, was %lu, expected %lu\n",			\
					__func__, range, expected);				\
		else										\
			pr_info("%s: bounds okay, was %lu, expected %lu\n",			\
					__func__, range, expected);				\
	 else											\
		if (range < expected || range > expected * 16)					\
			fail_dump("%s: BAD_BOUNDS, was %lu, expected %lu <= bound <= %lu\n",	\
					__func__, range, expected, expected*2);			\
		else										\
			pr_info("%s: bounds okay, was %lu, expected %lu <= bound <= %lu\n",	\
					__func__, range, expected, expected*2);			\
	} while (0)

#define MPXK_VERIFY_BOUNDS_FIVE(a,b,c,d,e,ae,be,ce,de,ee) do {	\
	verify_bounds(a, ae, false);				\
	verify_bounds(b, be, false);				\
	verify_bounds(c, ce, false);				\
	verify_bounds(d, de, false);				\
	verify_bounds(e, ee, false);				\
} while (0)

/*****************************************************************
 * dump_start
 * essentially a direct copy-paste of drivers/misc/lkdtm_bounds.c
 ****************************************************************/

/* #define test_attribute __attribute__((bnd_instrument)) noinline */
#define test_attribute noinline

noinline static
void verify_bounds(void *ptr, unsigned long expected, bool exact)
{ MPXK_VERIFY_BOUNDS(ptr, expected, exact); }


noinline static
void verify_bounds_arr(void **arr, int i, unsigned long expected)
{ verify_bounds(arr[i], expected, false); }

test_attribute
void lkdtm_MPX_BOUNDS_WRAPPER(void)
{
	int i;
	int p = 1;
	void *ptr;

	for (i = 0; i < 8; i++) {
		size_t size = 40 * p;
		pr_info("%s: verifying kmalloc(%lu, flags) instrumentation\n",
				__func__, size);

		/* This is instrumented and called via mpxk_wrapper_kmalloc */
		ptr = kmalloc(size, GFP_KERNEL);

		verify_bounds(ptr, size, true);

		kfree(ptr);
		p *= 2;
	}
}

#define CREATE_TEST_ARRAY(arr, i) do {				\
	for (i = 0; i < 10; i++)				\
		arr[i] = kmalloc(40 * (i+1), GFP_KERNEL);	\
} while (0);

#define CLEANUP_TEST_ARRAY(arr) do {	\
	for (i = 0; i < 10; i++)	\
		kfree(arr[i]);		\
} while (0)


test_attribute
void lkdtm_MPX_BOUNDS_LOAD(void)
{
	void *arr[10];
	int i;

	CREATE_TEST_ARRAY(arr, i);

	pr_info("%s: verifying pointers bounds via array\n", __func__);
	for (i = 0; i < 10; i++)
		verify_bounds_arr(arr, i, 40 * (i+1));

	pr_info("%s: cleaning up\n", __func__);
	CLEANUP_TEST_ARRAY(arr);
}

#define MAKE_FUNC_TEST(test_name, func) 				\
noinline static void FUNC_TEST_ ## func(void *arr[])			\
{									\
	func(arr[0], arr[1], arr[2], arr[3], arr[4], 			\
			40, 40 * 2, 40 * 3, 40 * 4, 40 * 5);		\
}									\
test_attribute void lkdtm_ ## test_name(void)			\
{									\
	void *arr[10];							\
	int i;								\
	CREATE_TEST_ARRAY(arr, i); 					\
	pr_info("testing func call instrumentation with " #func "\n");	\
	FUNC_TEST_##func(arr); 						\
	CLEANUP_TEST_ARRAY(arr);					\
}									\

#if 0
test_attribute void lkdtm_MPX_BOUNDS_FUNC_EXTERN(void) {  pr_info("%s: doing nothing\n", __func__); }
#else

extern void verify_bounds_five_extern (void *a, void *b, void *c, void *d, void *e,
	       unsigned long ae, unsigned long be, unsigned long ce,
	       unsigned long de, unsigned long ee);

MAKE_FUNC_TEST(MPX_BOUNDS_FUNC_EXTERN, verify_bounds_five_extern);
#endif

#if 0
test_attribute void lkdtm_MPX_BOUNDS_FUNC_STATIC(void) { pr_info("%s: doing nothing\n", __func__); }
#else

noinline static
void verify_bounds_five_static(void *a, void *b, void *c, void *d, void *e,
			unsigned long ae, unsigned long be, unsigned long ce,
			unsigned long de, unsigned long ee)
{ MPXK_VERIFY_BOUNDS_FIVE(a,b,c,d,e,ae,be,ce,de,ee); }

MAKE_FUNC_TEST(MPX_BOUNDS_FUNC_STATIC, verify_bounds_five_static);
#endif

#if 0
test_attribute void lkdtm_MPX_BOUNDS_FUNC_INLINE(void) { pr_info("%s: doing nothing\n", __func__); }
#else

static inline void verify_bounds_five_inlined(void *a, void *b, void *c, void *d, void *e,
		unsigned long ae, unsigned long be, unsigned long ce,
		unsigned long de, unsigned long ee)
{ MPXK_VERIFY_BOUNDS_FIVE(a,b,c,d,e,ae,be,ce,de,ee); }

MAKE_FUNC_TEST(MPX_BOUNDS_FUNC_INLINE, verify_bounds_five_inlined);
#endif

#if 0
test_attribute void lkdtm_MPX_BOUNDS_FUNC_NON_STATIC(void) { pr_info("%s: doing nothing\n", __func__); }
#else

void verify_bounds_five_non_static(void *a, void *b, void *c, void *d, void *e,
		unsigned long ae, unsigned long be, unsigned long ce,
		unsigned long de, unsigned long ee)
{ MPXK_VERIFY_BOUNDS_FIVE(a,b,c,d,e,ae,be,ce,de,ee); }

MAKE_FUNC_TEST(MPX_BOUNDS_FUNC_NON_STATIC, verify_bounds_five_non_static);
#endif


/**
 * mpxk_write_arr_i - Test function that writes to array.
 *
 * This has two functions, first it forces the array bounds to be passed correctly,
 * and second, it forces the string bounds to be loaded via BNDLDX.
 */
noinline static void mpxk_write_arr_i(char **arr, int i, int j, char c)
{
	arr[i][j] = c;
}

/**
 * mpxk_write_10_i - Test function that writes to function arg strings.
 *
 * TODO: Support other pointers, currently only void* ptrs supported as func args
 */
noinline static void mpxk_write_10_i(int i, int j, char c,
		void *s0, void *s1, void *s2, void *s3, void *s4,
		void *s5, void *s6, void *s7, void *s8, void *s9)
{
	if (i == 0) ((char *)s0)[j] = c;
	if (i == 1) ((char *)s1)[j] = c;
	if (i == 2) ((char *)s2)[j] = c;
	if (i == 3) ((char *)s3)[j] = c;
	if (i == 4) ((char *)s4)[j] = c;
	if (i == 5) ((char *)s5)[j] = c;
	if (i == 6) ((char *)s6)[j] = c;
	if (i == 7) ((char *)s7)[j] = c;
	if (i == 8) ((char *)s8)[j] = c;
	if (i == 9) ((char *)s9)[j] = c;
}

#define ARR_SIZE 40
#define BND_BASIC 0
#define BND_ARRAY 1
#define BND_FUNC10 2
#define SET_IJ(x) do { i = x; j = ARR_SIZE * (x+1) - 1; } while (0);

void do_the_test(const int test_num) {
	int i;
	int j;
	/* Wrapper should ensure we get bounds set for arr. */
	char **arr = (char **) kmalloc(10 * sizeof(char *), GFP_KERNEL);

	for (i = 0; i < 10; i++)
		/* BNDSTX will be needed to store all bounds. */
		arr[i] = (char *) kmalloc(ARR_SIZE * (i + 1), GFP_KERNEL);

	switch(test_num) {
		case BND_BASIC:
			/* Bad attempt should BT due to the arr bounds themselves.
			 * This happens provided that the normal wrapper is okay
			 * and the "normal" MPX bound passing is intact. */
			pr_info("attempting good ptr write\n");
			SET_IJ(0);
			mpxk_write_arr_i(arr, i, j, 'C');

			pr_info("attempting bad ptr write\n");
			mpxk_write_arr_i(arr, 10, 0, 'C');
			break;
		case BND_ARRAY:
			/* Tests bounds passed via function arguments. The
			 * offending bounds are in the strings, and thus need
			 * to be loaded via BNDLDX.
			 *
			 * The first bad attempt might succeed. */
			pr_info("attempting good ptr write\n");
			SET_IJ(2);
			mpxk_write_arr_i(arr, i, j, 'C');

			SET_IJ(3);
			pr_info("attempting exact (+1) bad ptr write (can succeed)");
			mpxk_write_arr_i(arr, i, j + 1, 'C');

			pr_info("attempting real bad ptr write (should be caught)\n");
			mpxk_write_arr_i(arr, i, j * 16, 'C');
			break;
		case BND_FUNC10:
			/* Tests bounds passed in via a function taking 10 strings.
			 * The bounds cannot be passed via bound registers, so the
			 * function argument passing will use BNDSTX+BNDSTX.
			 *
			 * The first bad attempt might succeed depending due to the
			 * mpxk load retrieving bounds based on MM metadata.
			 *
			 * The second bad attempt should hopefully be "bad enough". */
			pr_info("attempting good ptr write\n");
			SET_IJ(8);
			mpxk_write_10_i(2, 40 * 3 -1, 'C',
					arr[0], arr[1], arr[2], arr[3], arr[4],
					arr[5], arr[6], arr[7], arr[8], arr[9]);

			SET_IJ(7);
			pr_info("attempting exact bad ptr write\n");
			mpxk_write_10_i(2, 40 * 3 -1, 'C',
					arr[0], arr[1], arr[2], arr[3], arr[4],
					arr[5], arr[6], arr[7], arr[8], arr[9]);
			mpxk_write_arr_i(arr, 8, 40 * 9, 'C');

			SET_IJ(6);
			pr_info("attempting real bad ptr write\n");
			mpxk_write_arr_i(arr, 2, 40 * 16, 'C');
			break;
	}

	for (i = 0; i < 10; i++)
		kfree(arr[i]);
}

void lkdtm_MPXK_BND_BASIC(void) { do_the_test(BND_BASIC); }
void lkdtm_MPXK_BND_ARRAY(void) { do_the_test(BND_ARRAY); }
void lkdtm_MPXK_BND_FUNC10(void) { do_the_test(BND_FUNC10);}

/*****************************************************************
 * end of dump....
 ****************************************************************/

int test_dumps(void)
{
	printf("%s", __func__);

	lkdtm_MPX_BOUNDS_LOAD();
	lkdtm_MPX_BOUNDS_FUNC_EXTERN();
	lkdtm_MPX_BOUNDS_FUNC_INLINE();
	lkdtm_MPX_BOUNDS_FUNC_STATIC();
	lkdtm_MPX_BOUNDS_FUNC_NON_STATIC();
	lkdtm_MPX_BOUNDS_WRAPPER();
	/* lkdtm_MPXK_BND_BASIC(); */
	/* lkdtm_MPXK_BND_ARRAY(); */
	/* lkdtm_MPXK_BND_FUNC10(); */

	printf(" %s\n", (failed ? "FAILED" : "ok"));
	return !failed;
}

