/*
 * mpxk_builtins.c - Functions for MPXK "builtins" (wrappers)
 *
 * Defines various helper functions for identifying interesting functions
 * and converting names for wrapper functions.
 *
 * (Note: currently a bit messy, but will be cleaned up for "release".)
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"

/*
 * #define d(...) dsay(__VA_ARGS__)
 */
#define d(...)

#define CHKP_POSTFIX ".chkp"

static int wrapper_i(const char *name);
static int builtin_i(const char *name);
static bool mpxk_is_wrappable_chkp(const char *name);

struct mpxk_builtin {
	const bool is_wrapper;
	const bool is_loader;
	const char *name;
};

static const struct mpxk_builtin mpxk_fndecls[] = {
#define MPXK_BUILTIN_DEF(r,x,...) { .is_wrapper = 0, .is_loader = 1, .name = #x },
#define MPXK_WRAPPER_DEF(r,x,...) { .is_wrapper = 1, .is_loader = 0, .name = MPXK_WRAPPER_PREFIX #x },
#include "mpxk_builtins.def"
#undef MPXK_WRAPPER_DEF
#undef MPXK_BUILTIN_DEF
};

static int mpxk_fndecls_count = sizeof(mpxk_fndecls)/sizeof(mpxk_fndecls[0]);

bool mpxk_is_wrapper(const char *name)
{
	gcc_assert(name != NULL);
	return (bool) (wrapper_i(name) >= 0);
}

const char *mpxk_get_wrapper_name(const char *name)
{
	gcc_assert(name != NULL);

	char wr_name[strlen(MPXK_WRAPPER_PREFIX) + strlen(name) + 1];
	sprintf(wr_name, "%s%s", MPXK_WRAPPER_PREFIX, name);

	const int i = wrapper_i(wr_name);

	if (i >= 0)
		return mpxk_fndecls[i].name;

	return NULL;
}

bool mpxk_is_wrappable(const char *name)
{
	gcc_assert(name != NULL);
	return (bool) (mpxk_get_wrapper_name(name) != NULL);
}

static int builtin_i(const char *name) {
	gcc_assert(name != NULL);

	for (int i = 0; i < mpxk_fndecls_count; i++) {
		gcc_assert(mpxk_fndecls[i].name != NULL);

		if (0 == strcmp(mpxk_fndecls[i].name, name))
			return i;
	}
	d("%s not a builtint, -1\n", __FILE__, __LINE__, __func__, name);
	return -1;
}

static int wrapper_i(const char *name) {
	const int i = builtin_i(name);
	return (i == -1 ? -1 : (mpxk_fndecls[i].is_wrapper ? i : -1));
}

static bool mpxk_is_wrappable_chkp(const char *name)
{
	gcc_assert(name != NULL);
	const int len = strlen(name) - strlen(CHKP_POSTFIX);

	if (strstr(name, CHKP_POSTFIX) != (name + len))
		return false;

	char base_name[len+1] = {'\0'};
	memcpy(base_name, name, len);
	return mpxk_is_wrappable(base_name);
}

void mpxk_builitins_sanity_check(void)
{
	(void) gcc_version;
	gcc_assert(strlen(CHKP_POSTFIX) == 5);

	gcc_assert(0 == strcmp(MPXK_WRAPPER_PREFIX "kmalloc", mpxk_get_wrapper_name("kmalloc")));

	gcc_assert(builtin_i(MPXK_LOAD_BOUNDS_FN_NAME) >= 0);
	gcc_assert(builtin_i("mpxk_wrapper_kmalloc") >= 0);
	gcc_assert(mpxk_is_wrapper("mpxk_wrapper_kmalloc"));
	gcc_assert(mpxk_is_wrapper(MPXK_WRAPPER_PREFIX "kmalloc"));
	gcc_assert(mpxk_is_wrappable("kmalloc"));
	gcc_assert(mpxk_is_wrappable_chkp("kmalloc.chkp"));

	gcc_assert(!mpxk_is_wrapper( MPXK_LOAD_BOUNDS_FN_NAME));
	gcc_assert(builtin_i("kmalloc") < 0);
	gcc_assert(!mpxk_is_wrapper( "kmalloc"));
	gcc_assert(builtin_i( "mpxk_wrapper_not_good_at_all") < 0);
	gcc_assert(!mpxk_is_wrapper( "mpxk_wrapper_not_good_at_all"));
	gcc_assert(!mpxk_is_wrapper( "_" MPXK_WRAPPER_PREFIX "kmalloc"));
	gcc_assert(!mpxk_is_wrappable( MPXK_WRAPPER_PREFIX "kmalloc"));
	gcc_assert(!mpxk_is_wrappable_chkp( MPXK_WRAPPER_PREFIX "kmalloc.chkp"));
}

#undef d
