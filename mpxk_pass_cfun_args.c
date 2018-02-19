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
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"

#include <ipa-chkp.h>
#include <tree-chkp.h>

static unsigned int mpxk_cfun_args_execute(void);

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
	(void) gcc_version;
	return &pass_info_mpxk_cfun_args;
}

static unsigned int mpxk_cfun_args_execute(void)
{
	int bound_count = 0;
	int arg_count = 0;
	basic_block bb = ENTRY_BLOCK_PTR_FOR_FN(cfun)->next_bb;
	gimple_stmt_iterator iter = gsi_start_bb(bb);
	tree list = DECL_ARGUMENTS(cfun->decl);
	tree prev = NULL;

	if (skip_execute(BND_LEGACY)) return 0;

	if (list == NULL || list_length(list) == 0)
		return 0;

	tree *p;

	for (p = &list; *p; ) {
		tree l = *p;

		/* Keep count of the encountered bounds args */
		if (TREE_TYPE(l) == pointer_bounds_type_node) {
			bound_count++;

			iter = gsi_start_bb(bb);

			/* Add manual bound load for bounds passed via BNDSTX+BNDLDX.
			 * This happens in two scenarios handled here:
			 * 	- There are more bounds than available registers (>4)
			 * 	- The bounds are for arguments beyond the sixth argument,
			 * 	  this is *feature* seems to be related to how non-bound
			 * 	  arguments are treated (i.e. 6 args are passed via regs
			 * 	  before using the stack for arguments).
			 */
			if (bound_count > MPXK_BND_REGISTER_COUNT ||arg_count > 6) {
				dsay("resetting bound argument #%d (ptr > #6)", bound_count);
				gcc_assert(prev != NULL);
				insert_mpxk_bound_load(&iter, prev, l);
			}

			p = &TREE_CHAIN(l);
		} else {
			dsay("skipping non-bound argument #%d", arg_count);
			prev = l;
			arg_count++;
			p = &TREE_CHAIN(l);
		}

		/* The previous arg is needed if we need a MPXK load. */
		*p = TREE_CHAIN (l);
	}

	bound_count = (bound_count <= MPXK_BND_REGISTER_COUNT ? 0 :
			(bound_count - MPXK_BND_REGISTER_COUNT));

	mpxk_stats.cfun_ldx += bound_count;
	dsay("replaced %d bound args with mpkx_load_bounds", bound_count);
	return 0;
}

#undef d
