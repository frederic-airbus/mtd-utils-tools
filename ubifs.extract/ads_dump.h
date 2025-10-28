/**
 * AIRBUS DEFENCE & SPACE
 * October 2025
 * F. FRAYSSE
 */
#ifndef __ADS_DUMP_H__
#define __ADS_DUMP_H__


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>

#include "bitops.h"
#include "kmem.h"
#include "ubifs.h"
#include "defs.h"
#include "debug.h"
#include "key.h"
#include "misc.h"


/* ads_dump.c */
uint64_t ads_print_ino_node(struct ubifs_info *c, uint64_t inode);
void     ads_set_leb_to_dump(int leb);
void     ads_dump(struct ubifs_info *c);


/* peb_leb.c */
int peb_leb_init(const char *mtd_device); /* Call first */
int peb_leb_getPeb(int leb);
int peb_leb_getDataOffset(int peb);
int peb_leb_get_eb_size(void);

/* dump_fs.c */
void dump_fs_from_root(struct ubifs_info *c);

/* shrinker.c */
void shrinker_execute(struct ubifs_info *c);

#endif
