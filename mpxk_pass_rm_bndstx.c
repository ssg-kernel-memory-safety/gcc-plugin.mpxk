/*
 * mpxk_pass_rm_bndstx.c - RTL pass that remoes any BNDSTX insn
 *
 * This is a "brute force" approach that simply removes any encountered
 * BNDS insn.
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"
#include <rtl.h>
#include <print-rtl.h>

#define d(...) dsay(__VA_ARGS__)
/* #define d(...) */

static unsigned int mpxk_rm_bndstx_execute(void);
static bool insn_is_bndstx(rtx_insn *insn);

#define PASS_NAME mpxk_rm_bndstx
#define NO_GATE

#include "gcc-generate-rtl-pass.h"

static struct register_pass_info pass_info_mpxk_rm_bndstx = {
	.pass				= make_mpxk_rm_bndstx_pass(),
	.reference_pass_name		= "final",
	.ref_pass_instance_number	= 1,
	.pos_op				= PASS_POS_INSERT_BEFORE
};

struct register_pass_info *get_mpxk_rm_bndstx_pass_info(void)
{
	(void) gcc_version;
	return &pass_info_mpxk_rm_bndstx;
}

static unsigned int mpxk_rm_bndstx_execute(void)
{
	int start = mpxk_stats.dropped_stx_brute;
	expanded_location loc;
	basic_block bb, next;
	rtx_insn *insn;
	const char* name = DECL_NAME_POINTER(cfun->decl);

	if (skip_execute(BND_LEGACY)) return 0;

	loc = expand_location(DECL_SOURCE_LOCATION(current_function_decl));
	d("executing on %s\n", __FILE__, __LINE__, __func__, loc.file);

	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb;
	do {
		next = bb->next_bb;
		for (insn = BB_HEAD(bb); insn != BB_END(bb); insn = NEXT_INSN(insn)) {
			if (insn_is_bndstx(insn)) {
#ifdef MPXK_DEBUG
				loc = insn_location(insn);
				fprintf(stderr, "%s: removing bndstx in %s at %s:%d\n",
						__FILE__, DECL_NAME_POINTER(current_function_decl),
						loc.file, loc.line);
#endif /* MPXK_DEBUG */
				/* Our wrappers should not have this problem! */
				gcc_assert(!mpxk_is_wrap_any(name));
				remove_insn(insn);
				mpxk_stats.dropped_stx_brute++;
			}
		}
		bb = next;
	} while (bb);

	if (start == mpxk_stats.dropped_stx_brute) {
		loc = expand_location(DECL_SOURCE_LOCATION(current_function_decl));
		d("no changes made to %s\n", __FILE__, __LINE__, __func__, loc.file);
	}

	return 0;
}

static bool insn_is_bndstx(rtx_insn *insn) {
	rtx pattern, vec_el;
	int i;

	pattern = PATTERN(insn);

	/* FIXME: Potentially fragile, but should catch call arg bounds */
	if (pattern != NULL && GET_CODE(pattern) == PARALLEL) {
		for (i = 0; i < XVECLEN(pattern, 0); i++) {
			vec_el = XVECEXP(pattern, 0, i);

			if (i == 0 && GET_CODE(vec_el) == UNSPEC
			    && XVECLEN(vec_el, 0) > 0
			    && XINT(vec_el,1) == UNSPEC_BNDSTX) {
				return true;
			}
		}
	}

	return false;
}

#undef d
