/*
 * mpxk.h - MPXK plugin main header
 *
 * Copyright (C) 2017 Hans Liljestrand <LiljestrandH@gmail.com>
 *
 * This file is released under the GPLv2.
 */
#ifndef PLUGIN_MPX_PLUGIN_H
#define PLUGIN_MPX_PLUGIN_H

#include "gcc-common.h"

#define MPXK_BND_REGISTER_COUNT 4
#define MPXK_LOAD_BOUNDS_FN_NAME "mpxk_load_bounds"
#define MPXK_WRAPPER_PREFIX "mpxk_wrapper_"

/* #define MPXK_DEBUG */

struct mpxk_bound_store_stats {
	int dropped_ldx;
	int dropped_stx;
	int dropped_stx_brute;
	int wrappers_added;
};

/* Store some shared stats on current unit. */
extern struct mpxk_bound_store_stats mpxk_stats;

/* defined in mpxk.c */
void tree_list_drop_elements(tree list, bitmap dropped);
void insert_mpxk_bound_load(gimple_stmt_iterator *gsi, tree pointer, tree bounds);
void add_attribute(const char * mode, tree *attributes);

/* passes are defined in the mpxk_pass*.c files */
struct register_pass_info *get_mpxk_wrappers_pass_info(void);
struct register_pass_info *get_mpxk_wrap_funs_pass_info(void);
struct register_pass_info *get_mpxk_bnd_store_pass_info(void);
struct register_pass_info *get_mpxk_cfun_args_pass_info(void);
struct register_pass_info *get_mpxk_rm_bndstx_pass_info(void);

/* mpxk_builtins.c */
void mpxk_builitins_sanity_check(void);
bool mpxk_is_wrappable(const char *name);
bool mpxk_is_wrapper(const char *name);
bool mpxk_is_wrap_any(const char *name);
const char *mpxk_get_wrapper_name(const char *name);

/* TODO: Eventually remove these debugging macro printout thingies */
#ifdef MPXK_DEBUG
#define IF_MPXK_DEBUG(...) __VA_ARGS__
#define dsay_spacer " "
#define dsay(msg, ...) fprintf(stderr, "%s@%d~%s:" dsay_spacer msg, __VA_ARGS__)
#else
#define IF_MPXK_DEBUG(...)
#define dsay(...)
#endif /* MPXK_DEBUG */

#endif /* PLUGIN_MPX_PLUGIN_H */
