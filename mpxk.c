/*
 * mpxk.c - Common functionality and init for MPXK plugin
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"
#include <tree-chkp.h>
#include <ipa-chkp.h>

/*
 * #define d(f,...) dsay(f,__VA_ARGS__)
 */
#define d(...)

static void mpxk_plugin_finish(void *gcc_data, void *user_data);
static tree get_load_fndecl(void);

struct mpxk_bound_store_stats mpxk_stats = {0};

static tree init_bounds;

__visible int plugin_is_GPL_compatible;

static struct plugin_info mpxk_plugin_info = {
	.version	= "20170308",
	.help		= "MPX support for kernel space\n"
};

__visible int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
	const char * const plugin_name = plugin_info->base_name;
	int err;

	if (!plugin_default_version_check(version, &gcc_version)) {
		error(G_("incompatible gcc/plugin versions"));
		return 1;
	}

	/* First run some sanity checks... */
	mpxk_builitins_sanity_check();

	/* Register some generic plugin callbacks */
	register_callback(plugin_name, PLUGIN_INFO, NULL, &mpxk_plugin_info);
	register_callback(plugin_name, PLUGIN_FINISH, &mpxk_plugin_finish, NULL);

	/* Insert the specialized MPXK passes */

	/* Replace wrappables with mpxk_wrappers. */
	register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
			get_mpxk_wrappers_pass_info());

	/* Remove bndldx/bndstx calls.*/
	register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
			get_mpxk_bnd_store_pass_info());

	/* Handle incoming bounds arguments. */
	register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
			get_mpxk_cfun_args_pass_info());

	/* Brute force approach of just dropping all BNDSTX instructions.
	 * These get added in the RTL expand pass for function calls (and
	 * potentially other cases */
	register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
					  get_mpxk_rm_bndstx_pass_info());

	return 0;
}

/**
 * mpxk_plugin_finish - diplay some debug data on plugin finish.
 * Just dump some information on what we've done */
static void mpxk_plugin_finish(void *gcc_data, void *user_data)
{
	dsay("%d+%d BNDSTX, %d BNDLDX calls, %d wrappers\n", __FILE__, __LINE__, __func__,
			mpxk_stats.dropped_stx, mpxk_stats.dropped_stx_brute,
			mpxk_stats.dropped_ldx, mpxk_stats.wrappers_added);
}

/**
 * tree_list_drop_elements - remove specific elements from TREE_LIST.
 * @list: tree list
 * @dropped: bitmap of element indices to drop
 */
void tree_list_drop_elements(tree list, bitmap dropped)
{
	int i = 0;
	tree *p;

	if (list == NULL)
		return;

	for (p = &list; *p; i++) {
		tree l = *p;

		if (bitmap_bit_p(dropped, i)) {
			*p = TREE_CHAIN(l);
		} else {
			p = &TREE_CHAIN(l);
		}

		*p = TREE_CHAIN(l);
	}
}

/**
 * insert_mpxk_bound_load - insert all to the MPXK bound function and bndret.
 * @gsi: The gimple_stmt_iterator for the insert location.
 * @pointer: The pointer based on which the bounds are loaded.
 * @bounds: The bounds variable for storing the loaded bounds.
 *
 * This inserts two GIMPLE statemets, a gcall to the bounds load function and
 * a subsequent bndret to store the bounds. The iterator is left on the latter
 * bndret stmt.
 */
void insert_mpxk_bound_load(gimple_stmt_iterator *gsi, tree pointer, tree bounds)
{
	tree load_fndecl, tmp_ptr;
	gcall *load_call;

	/* Prepare our own mpxk load_bounds call, i.e. tmp_ptr = load_call(pointer) */
	load_fndecl = get_load_fndecl();
	load_call = gimple_build_call(load_fndecl, 1, pointer);

	/* We need the returned pointer for the latter bndret call */
	tmp_ptr = create_tmp_var(ptr_type_node, "tmp_ptr");
	gimple_call_set_lhs(load_call, tmp_ptr);

	/* Set chkp instrumentation stuff */
  	gimple_call_set_with_bounds(load_call, true);
  	gimple_set_plf(load_call, GF_PLF_1, false);

	/* Update load call statement */
	update_stmt(load_call);

	/* Some error checking */
	gcc_assert((load_call->subcode & GF_CALL_INTERNAL) == 0);
	gcc_assert ((load_call->subcode & GF_CALL_WITH_BOUNDS) != 0);

	gsi_insert_before(gsi, load_call, GSI_NEW_STMT);
	chkp_insert_retbnd_call(bounds, tmp_ptr, gsi);

	/* Are these needed since we work "beyond" chkp? */
	chkp_set_bounds(pointer, bounds);
	gimple_call_with_bounds_p(load_call);

	update_stmt(load_call);
}

/**
 * get_load_fndecl - Return fndecl for the MPXK bounds load function
 */
static tree get_load_fndecl(void)
{
	/* d("building load_fndecl\n", __FILE__, __LINE__, __func__); */
	tree parm_type_list, fntype, fndecl;

	/* fndecl argument/parameter list */
	parm_type_list = build_tree_list_stat(NULL_TREE, ptr_type_node);

	fntype = build_function_type(ptr_type_node, parm_type_list);
	fntype = chkp_copy_function_type_adding_bounds(fntype);

	fndecl = build_decl(UNKNOWN_LOCATION, FUNCTION_DECL,
			get_identifier(MPXK_LOAD_BOUNDS_FN_NAME), fntype);

	DECL_EXTERNAL(fndecl) = 1;

	DECL_RESULT(fndecl) = build_decl(UNKNOWN_LOCATION, RESULT_DECL,
			NULL_TREE, ptr_type_node);

	DECL_ATTRIBUTES(fndecl) = tree_cons(get_identifier("chkp instrumented"),
			NULL, DECL_ATTRIBUTES(fndecl));

	TREE_PUBLIC(DECL_RESULT(fndecl)) = 1;

	return fndecl;
}

/**
 *
 * @param mode
 * @param attributes
 */
void add_attribute(const char * mode, tree *attributes)
{
	size_t len = strlen (mode);
	tree value = build_string(len, mode);

	TREE_TYPE (value) = build_array_type (char_type_node,
			build_index_type (size_int (len)));

	*attributes = tree_cons (get_identifier ("target"),
			build_tree_list (NULL_TREE, value),
			*attributes);
}

#undef d
