/*
 * mpxk_pass_sweeper.c - RTL pass for finding missed BND{STX,LDX}
 *
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"
#include <rtl.h>
#include <print-rtl.h>

static unsigned int mpxk_sweeper_execute(void);
static bool contains_unspec(rtx pattern, const int code);

#define PASS_NAME mpxk_sweeper
#define NO_GATE

#include "gcc-generate-rtl-pass.h"

static struct register_pass_info pass_info_mpxk_sweeper = {
	.pass				= make_mpxk_sweeper_pass(),
	.reference_pass_name		= "final",
	.ref_pass_instance_number	= 1,
	.pos_op				= PASS_POS_INSERT_BEFORE
};

struct register_pass_info *get_mpxk_sweeper_pass_info(void)
{
	(void) gcc_version;
	return &pass_info_mpxk_sweeper;
}

static unsigned int mpxk_sweeper_execute(void)
{
	expanded_location loc;
	basic_block bb, next;
	rtx_insn *insn;
	rtx r;
	int found = 0;

	if (skip_execute(NULL)) return 0;

	loc = expand_location(DECL_SOURCE_LOCATION(current_function_decl));

	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb;
	do {
		next = bb->next_bb;
		for (insn = BB_HEAD(bb); insn != BB_END(bb); insn = NEXT_INSN(insn)) {
			r = PATTERN(insn);
			if (INSN_LOCATION(insn) != UNKNOWN_LOCATION)
				loc = insn_location(insn);

			if (r != NULL) {
				if (contains_unspec(r, UNSPEC_BNDSTX)) {
					dsay("SWEEPER_WARNING => removed bndstx at %s:%d",
							loc.file, loc.line);
					delete_insn(insn);
					mpxk_stats.sweep_stx++;
					found++;
				}
				if (contains_unspec(r, UNSPEC_BNDLDX) ||
				    contains_unspec(r, UNSPEC_BNDLDX_ADDR)) {
					dsay("SWEEPER_WARNING => removed bndldx at %s:%d",
							loc.file, loc.line);
					delete_insn(insn);
					mpxk_stats.sweep_ldx++;
					found++;
				}
			}
		}
		bb = next;
	} while (bb);

	loc = expand_location(DECL_SOURCE_LOCATION(current_function_decl));
	return 0;
}

static bool contains_unspec(rtx r, const int code) {
	int i;

	gcc_assert(r != NULL);

	if (GET_CODE(r) == UNSPEC || GET_CODE(r) == UNSPEC_VOLATILE) {
		if (XINT(r, 1) == code) {
			return true;
		}
	} else if (GET_CODE(r) == PARALLEL || GET_CODE(r) == SEQUENCE) {
		for (i = 0; i < XVECLEN(r, 0); i++) {
			if (contains_unspec(XVECEXP(r, 0, i), code))
				return true;
		}
	} else if (GET_CODE(r) == UNSPEC || GET_CODE(r) == UNSPEC_VOLATILE) {
		if (XINT(r, 1) == code) {
			return true;
		}
	} else if (GET_CODE(r) == SET) {
		if (contains_unspec(SET_SRC(r), code))
			return true;
		if (contains_unspec(SET_DEST(r), code))
			return true;
	}

	return false;
}

#undef d
