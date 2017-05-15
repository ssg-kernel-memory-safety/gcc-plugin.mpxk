/*
 * mpxk_pass_sweeper.c - RTL pass for finding missed BND{STX,LDX}
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#include "mpxk.h"
#include <rtl.h>
#include <print-rtl.h>

#define d(...) dsay(__VA_ARGS__)

static unsigned int mpxk_sweeper_execute(void);
static bool contains_unspec(rtx pattern, const int code);

#define PASS_NAME mpxk_sweeper
#define NO_GATE

#include "gcc-generate-rtl-pass.h"

static struct register_pass_info pass_info_mpxk_sweeper = {
	.pass				= make_mpxk_sweeper_pass(),
	.reference_pass_name		= "final",
	.ref_pass_instance_number	= 1,
	.pos_op				= PASS_POS_INSERT_AFTER
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
	d("starting final sweep\n", __FILE__, __LINE__, __func__);

	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb;
	do {
		next = bb->next_bb;
		for (insn = BB_HEAD(bb); insn != BB_END(bb); insn = NEXT_INSN(insn)) {
			r = PATTERN(insn);
			if (INSN_LOCATION(insn) != UNKNOWN_LOCATION)
				loc = insn_location(insn);

			if (r != NULL) {
				if (contains_unspec(r, UNSPEC_BNDSTX)) {
#ifndef MPXK_SWEEPER_DO_REMOVE
					fprintf(stderr, "SWEEPER_ERROR: found bndstx in %s at %s:%d:%s\n",
							DECL_NAME_POINTER(current_function_decl),
							loc.file, loc.line, DECL_NAME_POINTER(current_function_decl));
#else /* MPXK_SWEEPER_DO_REMOVE */
					fprintf(stderr, "SWEEPER_WARNING: found bndstx in %s at %s:%d:%s\n",
							DECL_NAME_POINTER(current_function_decl),
							loc.file, loc.line, DECL_NAME_POINTER(current_function_decl));
					remove_insn(insn);
#endif /* MPXK_SWEEPER_DO_REMOVE */
					mpxk_stats.sweep_stx++;
					found++;
				}
				if (contains_unspec(r, UNSPEC_BNDLDX) ||
						contains_unspec(r, UNSPEC_BNDLDX_ADDR)) {
#ifndef MPXK_SWEEPER_DO_REMOVE
					fprintf(stderr, "SWEEPER_ERROR: found bndldx in %s at %s:%d:%s\n",
							DECL_NAME_POINTER(current_function_decl),
							loc.file, loc.line, DECL_NAME_POINTER(current_function_decl));
#else /* MPXK_SWEEPER_DO_REMOVE */
					fprintf(stderr, "SWEEPER_WARNING: found bndldx in %s at %s:%d:%s\n",
							DECL_NAME_POINTER(current_function_decl),
							loc.file, loc.line, DECL_NAME_POINTER(current_function_decl));
					remove_insn(insn);
#endif /* MPXK_SWEEPER_DO_REMOVE */
					mpxk_stats.sweep_ldx++;
					found++;
				}
			}
		}
		bb = next;
	} while (bb);

	loc = expand_location(DECL_SOURCE_LOCATION(current_function_decl));
	d("sweep %d problems in %s\n", __FILE__, __LINE__, __func__, found, loc.file);

#ifdef MPXK_CRASH_ON_SWEEP
	if (found)
		internal_error("Unhandled bndstx/bndldx instructions in %s:%d:%s\n",
				loc.file, loc.line, DECL_NAME_POINTER(current_function_decl));
#endif /* MPXK_CRASH_ON_SWEEP */

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
