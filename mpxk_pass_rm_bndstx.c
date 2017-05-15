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
/*
 * #define d(...)
 */

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
	return &pass_info_mpxk_rm_bndstx;
}

static unsigned int mpxk_rm_bndstx_execute(void)
{
	basic_block bb, next;
	rtx_insn *insn;
	const char* name = DECL_NAME_POINTER(cfun->decl);

	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb;
	do {
		next = bb->next_bb;
		for (insn = BB_HEAD(bb); insn != BB_END(bb); insn = NEXT_INSN(insn)) {
			if (insn_is_bndstx(insn)) {
				/* Our wrappers should not have this problem! */
				gcc_assert(!mpxk_is_wrap_any(name));
				remove_insn(insn);
				mpxk_stats.dropped_stx_brute++;
			}
		}
		bb = next;
	} while (bb);

	return 0;
}

static bool insn_is_bndstx(rtx_insn *insn) {
	rtx pattern;
	rtx vec_el;
	int i;
	int l;

	pattern = PATTERN(insn);

	/* TODO: This is *very* crude and probably fragile */
	if (pattern != NULL && GET_CODE(pattern) == PARALLEL) {
		for (i = 0; i < XVECLEN(pattern, 0); i++) {
			vec_el = XVECEXP(pattern, 0, i);
			int l = XVECLEN(vec_el, 0);

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
