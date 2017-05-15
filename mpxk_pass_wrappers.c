/*
 * mpxk_pass_wrappers.c - A pre-chkp pass that inserts MPXK wrappers
 *
 * This pass replaces wrappable function calls with the appropriate MPXK
 * wrapper functions.
 *
 * Note: Function pointers might need some special care, but are probably
 *  not that common for the wrappable functions.
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"
#include <ipa-chkp.h>

/*
 * #define d(...) dsay(__VA_ARGS__)
 */
#define d(...)

static unsigned int mpxk_wrappers_execute(void);
static void mpxk_wrappers_gimple_call(gimple_stmt_iterator *gsi);
static void replace_gimple_call_static(gcall *call, const tree fndecl);

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
	return &pass_info_mpxk_wrappers;
}

static unsigned int mpxk_wrappers_execute(void)
{
	basic_block bb, next;
	gimple stmt;
	gimple_stmt_iterator iter;

	const tree fndecl = cfun->decl;
	const char* name = DECL_NAME_POINTER(fndecl);

	/* Do not modify wrapper functions */
	if (mpxk_is_wrapper(name))
		return 0;

	/* Do not modify bnd_legacy functions */
	if (lookup_attribute("bnd_legacy", DECL_ATTRIBUTES(fndecl)) != NULL)
		return 0;

	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb;
	do {
		next = bb->next_bb;
		for (iter = gsi_start_bb(bb); !gsi_end_p(iter); ) {
			stmt = gsi_stmt(iter);

			switch (gimple_code(stmt)) {
			case GIMPLE_CALL:
				mpxk_wrappers_gimple_call(&iter);
				break;
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
	gcc_assert(!mpxk_is_wrapper(DECL_NAME_POINTER(cfun->decl)));

	tree arg, type, new_decl, fndecl;
	gcall *call = as_a <gcall *> (gsi_stmt (*gsi));
	const char *new_name;
	fndecl = gimple_call_fndecl(call);

	if (!fndecl || !mpxk_is_wrappable(DECL_NAME_POINTER(fndecl)))
		return;

	new_decl = copy_node(fndecl);
	new_name = mpxk_get_wrapper_name(DECL_NAME_POINTER(fndecl));

	gcc_assert(new_name != NULL);

	DECL_VISIBILITY(new_decl) = VISIBILITY_DEFAULT;
	DECL_NAME(new_decl) = get_identifier(new_name);
	SET_DECL_ASSEMBLER_NAME(new_decl, get_identifier(new_name));

	/* Copy function arguments */
	DECL_ARGUMENTS(new_decl) = copy_list(DECL_ARGUMENTS(fndecl));
	for (arg = DECL_ARGUMENTS(new_decl); arg; arg = DECL_CHAIN(arg))
		DECL_CONTEXT(arg) = new_decl;

	/* Copy and modify function attributes */
	/* TODO: Experiment with allowing inlined wrappers. */
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

	d("swapped %s to %s in %s\n", __FILE__, __LINE__, __func__,
			DECL_NAME_POINTER(fndecl), new_name,
			DECL_NAME_POINTER(cfun->decl));

	mpxk_stats.wrappers_added++;
}

#undef d
