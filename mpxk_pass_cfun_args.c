/*
 * mpxk_pass_cfun_args.c - MPXK pass for cfun arguments
 *
 * Simple pass that only checks the current function (cfun) arguments for
 * pointer bound counts that exceed the available bndreg count. Such bounds
 * are then manually initialized at function start using the MPXK bound
 * load function.
 *
 * FIXME: Currently only handles (void *) ptr arguments!
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"

#include <ipa-chkp.h>
#include <tree-chkp.h>

static unsigned int mpxk_cfun_args_execute(void);

/*
 * #define d(...) dsay(__VA_ARGS__)
*/
#define d(...)

#define PASS_NAME mpxk_cfun_args
#define NO_GATE

#include "gcc-generate-gimple-pass.h"

static struct register_pass_info pass_info_mpxk_cfun_args = {
	.pass				= make_mpxk_cfun_args_pass(),
	.reference_pass_name		= "optimized",
	.ref_pass_instance_number	= 1,
	.pos_op				= PASS_POS_INSERT_BEFORE
};

struct register_pass_info *get_mpxk_cfun_args_pass_info(void)
{
	return &pass_info_mpxk_cfun_args;
}


static unsigned int mpxk_cfun_args_execute(void)
{
	int bound_count = 0;
	const char* name = DECL_NAME_POINTER(cfun->decl);
	basic_block bb = ENTRY_BLOCK_PTR_FOR_FN(cfun)->next_bb;
	gimple_stmt_iterator iter = gsi_start_bb(bb);
	tree list = DECL_ARGUMENTS(cfun->decl);
	tree prev = NULL;

	if (list == NULL || list_length(list) == 0)
		return 0;


	tree *p;

	for (p = &list; *p; ) {
		tree l = *p;

		/* Keep count of the encountered bounds args */
		if (TREE_TYPE(l) == pointer_bounds_type_node)
			bound_count++;

		/* Process only bounds becyond the available register count */
		if (bound_count > MPXK_BND_REGISTER_COUNT &&
				TREE_TYPE(l) == pointer_bounds_type_node) {
			d("resetting bound argument #%d\n", __FILE__, __LINE__, __func__, bound_count);

			/* Check that the bounds were preceeded by a pointer */
			gcc_assert(prev != NULL && TREE_TYPE(prev) == ptr_type_node);

			/* Insert MPXK bound load for the encountered bounds */
			insert_mpxk_bound_load(&iter, prev, l);

			/* Drop the bound from DECL_ARGUMENTS */
			*p = TREE_CHAIN(l); /* TODO: Do we need to do this? */
		} else {
			p = &TREE_CHAIN(l);
		}

		/* The previous arg is needed if we need a MPXK load. */
		prev = l;
		*p = TREE_CHAIN (l);
	}

	if (bound_count > MPXK_BND_REGISTER_COUNT)
		d("dropped %d bounds from cfun %s\n", __FILE__, __LINE__, __func__,
				bound_count - MPXK_BND_REGISTER_COUNT, name);

	return 0;
}

#undef d
