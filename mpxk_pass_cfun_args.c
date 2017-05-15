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
/* static void insert_bug_guard_assign(gimple_stmt_iterator *gsi, tree pointer, tree bounds); */

#define d(...) dsay(__VA_ARGS__)
/* #define d(...) */

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

			/* Process only bounds becyond the available register count */
			if (bound_count > MPXK_BND_REGISTER_COUNT) {
				d("resetting bound argument #%d for %s\n",
						__FILE__, __LINE__, __func__, bound_count,
						DECL_NAME_POINTER(current_function_decl));

				gcc_assert(prev != NULL);

				/* Insert MPXK bound load for the encountered bounds */
				insert_mpxk_bound_load(&iter, prev, l);

			} else if (arg_count > 6) {
				/* This seems to be a bug/feature. If a poinrter is becyond
				 * the sixth (non-bound) argument, then we're using bndstx
				 * bndldx whether or not registers would be available. */
				d("resetting bound argument #%d for %s (ptr > #6)\n",
						__FILE__, __LINE__, __func__, bound_count,
						DECL_NAME_POINTER(current_function_decl));
				gcc_assert(prev != NULL);
				insert_mpxk_bound_load(&iter, prev, l);
#ifdef MPXK_DEBUG
			} else {
				d("skipping bound argument %s\n", __FILE__, __LINE__, __func__,
						DECL_NAME_POINTER(current_function_decl));
#endif /* MPXK_DEBUG */
			}

			/* Leave the arg, optimizer should fix this */
			/* *p = TREE_CHAIN(l); */
			p = &TREE_CHAIN(l);
		} else {
			d("skipping argument for %s\n", __FILE__, __LINE__, __func__,
						DECL_NAME_POINTER(current_function_decl));
			prev = l;
			arg_count++;
			p = &TREE_CHAIN(l);
		}

		/* The previous arg is needed if we need a MPXK load. */
		*p = TREE_CHAIN (l);
	}

	int drop_count = (bound_count - MPXK_BND_REGISTER_COUNT);

	if (drop_count > 0) {
		mpxk_stats.cfun_ldx += drop_count;
		d("dropped %d bounds from cfun %s\n", __FILE__, __LINE__, __func__,
				drop_count, name);
#ifdef MPXK_DEBUG
		expanded_location loc = expand_location(DECL_SOURCE_LOCATION(current_function_decl));
		fprintf(stderr, "%s: replaced %d bound args in %s declaration at %s:%d\n",
				__FILE__, drop_count, name, loc.file, loc.line);
#endif /* MPXK_DEBUG */
	}
	return 0;
}

/* BUG in GCC mpx/chkp!?!
 *
 * net/ipv4/tcp_input.c:
 *
 *
static u8 tcp_sacktag_one(struct sock *sk,
			  struct tcp_sacktag_state *state, u8 sacked,
			  u32 start_seq, u32 end_seq,
			  int dup_sack, int pcount,
			  const struct skb_mstamp *xmit_time)



(insn:TI 15 23 567 2 (parallel [
            (set (reg:BND64 78 bnd1 [orig:195 __chkp_bounds_of_xmit_time ] [195])
                (unspec:BND64 [
                        (mem:DI (unspec:DI [
                                    (plus:DI (reg/f:DI 6 bp)
                                        (const_int 24 [0x18]))
                                    (reg/f:DI 0 ax [orig:197 xmit_time ] [197])
                                ] UNSPEC_BNDLDX_ADDR) [0  S8 A8])
                    ] UNSPEC_BNDLDX))
            (use (mem:BLK (plus:DI (reg/f:DI 6 bp)
                        (const_int 24 [0x18])) [0  A8]))
        ]) net/ipv4/tcp_input.c:1197 1106 {*bnd64_ldx}
     (expr_list:REG_DEAD (reg/f:DI 0 ax [orig:197 xmit_time ] [197])
        (nil)))
*/

/*
static void insert_bug_guard_assign(gimple_stmt_iterator *gsi, tree pointer, tree bounds)
{
	if (0 != strcmp("tcp_sacktag_one.chkp", DECL_NAME_POINTER(current_function_decl)))
		return;
	d("APPLYING WORKAROUND TO %s\n", __FILE__, __LINE__, __func__,
			DECL_NAME_POINTER(current_function_decl));

	gassign *assign;
	tree safe_bounds;

	gcc_assert(TREE_TYPE(bounds) == pointer_bounds_type_node);

    	safe_bounds = create_tmp_reg (pointer_bounds_type_node, "cfun_safe_bnd");
	assign = gimple_build_assign(safe_bounds, bounds);

	gsi_insert_before(gsi, assign, GSI_CONTINUE_LINKING);
	chkp_set_bounds(pointer, safe_bounds);
	update_stmt(assign);
}
*/


#undef d
