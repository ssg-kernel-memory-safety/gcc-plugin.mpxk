/*
 * mpxk_pass_wrappers.c - A pre-chkp pass that inserts MPXK wrappers
 *
 * This pass replaces wrappable function calls with the appropriate MPXK
 * wrapper functions.
 *
 * Note: Function pointers might need some special care, but are probably
 *  not that common for the wrappable functions.
 *
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"
#include <ipa-chkp.h>

static unsigned int mpxk_wrappers_execute(void);
static void mpxk_wrappers_gimple_call(gimple_stmt_iterator *gsi);

#define PASS_NAME mpxk_wrappers
#define NO_GATE

#include "gcc-generate-gimple-pass.h"

struct register_pass_info pass_info_mpxk_wrappers = {
	.pass				= make_mpxk_wrappers_pass(),
	.reference_pass_name 		= "cfg",
	.ref_pass_instance_number 	= 1,
	.pos_op				= PASS_POS_INSERT_AFTER
};

struct register_pass_info *get_mpxk_wrappers_pass_info(void)
{
	(void) gcc_version;
	return &pass_info_mpxk_wrappers;
}

static unsigned int mpxk_wrappers_execute(void)
{
	tree fndecl;
	basic_block bb, next;
	gimple_stmt_iterator iter;
	gimple stmt;

	if (skip_execute(BND_LEGACY)) return 0;

	/* Do not modify wrapper functions */
	if (mpxk_is_wrapper(DECL_NAME_POINTER(cfun->decl)))
		return 0;

	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb;
	do {
		next = bb->next_bb;
		for (iter = gsi_start_bb(bb); !gsi_end_p(iter); ) {
			stmt = gsi_stmt(iter);

			if (gimple_code(stmt) == GIMPLE_CALL) {
				fndecl = gimple_call_fndecl(as_a <gcall *>(stmt));
				if (fndecl && mpxk_is_wrappable(DECL_NAME_POINTER(fndecl))) {
					dsay("inserting wrapper for %s", DECL_NAME_POINTER(fndecl));
					mpxk_wrappers_gimple_call(&iter);
				}
			}

			gsi_next(&iter);
		}
		bb = next;
	}
	while (bb);

	return 0;
}

/**
 * mpxk_wrappers_gimple_call - Replace wrappables with wrappers.
 * @param gsi
 *
 * Based on gcc/tree-chkp.c ~ chkp_add_bounds_to_call_stmt
 */
static void mpxk_wrappers_gimple_call(gimple_stmt_iterator *gsi)
{
	tree arg, type, new_decl, fndecl;
	const char *new_name, *name;
	gcall *call;

	/* Get the current data */
	call = as_a <gcall *>(gsi_stmt (*gsi));
	fndecl = gimple_call_fndecl(call);
	name = DECL_NAME_POINTER(fndecl);

	/* Create data for new call */
	new_decl = copy_node(fndecl);
	new_name = mpxk_get_wrapper_name(name);
	gcc_assert(new_name != NULL);

	/* Set visibility. TODO: do we need this? */
	DECL_VISIBILITY(new_decl) = VISIBILITY_DEFAULT;

	/* Set wrapper name */
	DECL_NAME(new_decl) = get_identifier(new_name);
	SET_DECL_ASSEMBLER_NAME(new_decl, get_identifier(new_name));

	/* Copy function arguments */
	DECL_ARGUMENTS(new_decl) = copy_list(DECL_ARGUMENTS(fndecl));
	for (arg = DECL_ARGUMENTS(new_decl); arg; arg = DECL_CHAIN(arg))
		DECL_CONTEXT(arg) = new_decl;

	/* Copy and modify function attributes */
	DECL_ATTRIBUTES(new_decl) = remove_attribute("always_inline",
			copy_list(DECL_ATTRIBUTES(fndecl)));

	/* Mark the funciton external */
	DECL_EXTERNAL(new_decl) = 1;

	gimple_call_set_fndecl(call, new_decl);

	/* TODO: Double check if manual fntype bounds add is needed. */
	type = gimple_call_fntype(call);
	type = chkp_copy_function_type_adding_bounds(type);
	gimple_call_set_fntype(call, type);

	update_stmt(call);

	mpxk_stats.wrappers_added++;

	dsay("inserted %s at %s:%d\n", new_name, gimple_filename(call), gimple_lineno(call));
}

#undef d
