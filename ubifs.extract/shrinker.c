// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2025 AIRBUS Defence & Space
 *
 * Authors: Frederic Fraysse
 */

#include "ads_dump.h"

/**
 * Clean all TNC index
 * When traversing node's, libubifs allocate area for znode or zbranch
 * In real life with linux driver, using node age, node are cleaned
 * So here forcing the clean of all node
 */
void shrinker_execute(struct ubifs_info *c)
{
	long n;

	ubifs_destroy_tnc_tree(c);

	/* Reset the clean zone counter */
	n = atomic_long_read(&c->clean_zn_cnt);
	atomic_long_sub(n, &c->clean_zn_cnt);

	printf("Freing znode (%ld freed)\n", n);
}

