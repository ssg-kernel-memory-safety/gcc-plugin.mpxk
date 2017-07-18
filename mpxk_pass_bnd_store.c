/*
 * mpxk_pass_bnd_store.c - Handle basic bndstx/bndldx cases.
 *
 * This GIMPLE, post chkp, pass removes any encountered bndstx and bndldx
 * calls. The bndldx calls are replaces with calls to the MPXK bound load
 * function. Note that this only catches statements added during the chkp
 * GIMPLE passes, and does not handle stores and loads due to function
 * arguments (and potentially other cases).
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"

#include <ipa-chkp.h>
#include <tree-chkp.h>

static unsigned int mpxk_bnd_store_execute(void);

static void handle_ldx(gimple_stmt_iterator *gsi, gcall *call);
static void handle_stx(gimple_stmt_iterator *gsi, gcall *call);

#define PASS_NAME mpxk_bnd_store

#define NO_GATE

#include "gcc-generate-gimple-pass.h"

static struct register_pass_info pass_info_mpxk_bnd_store = {
	.pass				= make_mpxk_bnd_store_pass(),
	.reference_pass_name		= "optimized",
	.ref_pass_instance_number	= 1,
	.pos_op				= PASS_POS_INSERT_BEFORE
};

struct register_pass_info *get_mpxk_bnd_store_pass_info(void)
{
	(void) gcc_version;
	return &pass_info_mpxk_bnd_store;
}

static unsigned int mpxk_bnd_store_execute(void)
{
	basic_block bb, next;
	gimple stmt;
	gimple_stmt_iterator iter;
	gcall *call;
	tree fndecl;

	if (skip_execute(BND_LEGACY)) return 0;

	const char* name = DECL_NAME_POINTER(cfun->decl);

	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb;
	do {
		next = bb->next_bb;
		for (iter = gsi_start_bb(bb); !gsi_end_p(iter); ) {
			stmt = gsi_stmt(iter);

			switch (gimple_code(stmt)) {
			case GIMPLE_CALL:
				call = as_a<gcall *>(gsi_stmt(iter));
				fndecl = gimple_call_fndecl(call);

				/* Sanity check, the wrapper function should never end up here! */
				gcc_assert(!mpxk_is_wrapper(name));

				if (!fndecl)
					break;

				if (!strcmp(DECL_NAME_POINTER(fndecl), "__builtin_ia32_bndstx"))
					handle_stx(&iter, call);
				else if (!strcmp(DECL_NAME_POINTER(fndecl), "__builtin_ia32_bndldx"))
					handle_ldx(&iter, call);

				break;
			default:
				break;
			}

			gsi_next(&iter);
		}
		bb = next;
	} while (bb);

	return 0;
}

/**
 * handle_stx - Remove bndstx statement at iterator.
 * @gsi - Statement iterator.
 * @call - The call to remove.
 */
static void handle_stx(gimple_stmt_iterator *gsi, gcall *call)
{
	dsay("removed bndstx call in at %s:%d\n", gimple_filename(call), gimple_lineno(call));
	gcc_assert(!strcmp(DECL_NAME_POINTER(gimple_call_fndecl(call)),
				"__builtin_ia32_bndstx"));

	/* Remove stmt and update iterator so next item is correct */
	gsi_remove(gsi, true);
	gsi_prev(gsi);

	unlink_stmt_vdef(call);

	mpxk_stats.dropped_stx++;
}

/**
 * handle_ldx - Remove bndldx and replace it with MPXK bound load
 * @gsi - Statement iterator.
 * @call - The call to remove.
 *
 * We want to remove:
 * 	bounds = bndldx(orig_ptr)
 * And insert:
 * 	tmp_ptr = load_bounds(orig_ptr)
 * 	bounds = bndret(tmp_ptr)
 */
static void handle_ldx(gimple_stmt_iterator *gsi, gcall *call)
{
	tree orig_ptr, bounds;

	dsay("replaced bndldx call in at %s:%d", gimple_filename(call), gimple_lineno(call));
	gcc_assert(!strcmp(DECL_NAME_POINTER(gimple_call_fndecl(call)),
				"__builtin_ia32_bndldx"));

	/* Store what we need from the bndldx_call */
	orig_ptr = gimple_call_arg(call, 1);
	bounds = gimple_call_lhs(call);

	/* Remove the bndldx call, which moves iterator to next stmt. */
	gsi_remove(gsi, true);
	unlink_stmt_vdef(call);

	/* Now insert our own load bounds function */
	insert_mpxk_bound_load(gsi, orig_ptr, bounds);

	/* Make sure iterator points to last statement we worked on */
	gsi_prev(gsi);

	mpxk_stats.dropped_ldx++;
}

#undef d
