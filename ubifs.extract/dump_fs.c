/**
 * AIRBUS DEFENCE & SPACE
 * October 2025
 * F. FRAYSSE
 */

#include "ads_dump.h"

#include "linux_err.h"

#define FILE_SEP "/"
#define STR_LEN (300)

/**
 * Dump LEB & PEB of a file.
 */
static void dump_file(struct ubifs_info *c, uint64_t inode)
{
	uint64_t inoSize;
	unsigned int blockNum;
	union ubifs_key key;
	int n;
	struct ubifs_znode *znode;
	int lnum, lastLnum;
	uint64_t len;
	int ret;
	int nbPrint;
	int dirtyLine;
	int hole_block;

	/* mode of file... */
	inoSize = ads_print_ino_node(c, inode);


	blockNum   =  0;
	len        =  0;
	lastLnum   = -1;
        nbPrint    =  0;
	dirtyLine  =  0;
	hole_block = -1;
	while (inoSize > (blockNum*UBIFS_BLOCK_SIZE) )
	{
		/* Generate a data key */
		data_key_init(c, &key, inode, blockNum);
		/* Find a level 0 data node */
		ret = ubifs_lookup_level0(
					c,
					&key,
                                        &znode,
					&n);
		/* Entry not found, hole in file, block with data set to zero */
		if ( (ret != 1) && (hole_block == -1) )
		{
			hole_block = blockNum;
		}

		/* Entry found and it was an hole (return to a normal situation) */
		if ( (ret == 1) && (hole_block != -1) )
		{
			if (dirtyLine)
			{
				printf("\n");
				dirtyLine = 0;
			}
			/* Print the hole area */
			printf("No Entry from 0x%llX to 0x%llX (sparse area?zero in file)\n", 
				((uint64_t)hole_block)*UBIFS_BLOCK_SIZE,
				((uint64_t)blockNum)*UBIFS_BLOCK_SIZE - 1);
			/* End of the hole */
			hole_block = -1;
		}

		/* No entry found */
		if (ret != 1)
		{
			blockNum++;
			continue;
		}

		/* Use information provided by the lookup function */
		lnum  = znode->zbranch[n].lnum;
		len  += znode->zbranch[n].len - offsetof(struct ubifs_data_node,data);

		/* Many data node in the same LEB, print LEB # only one time */
		if (lnum != lastLnum)
		{
			printf("(LEB:%d,PEB:%d) ", lnum, peb_leb_getPeb(lnum));
			dirtyLine = 1;

			lastLnum = lnum;

	                if ( (nbPrint>0) && ( (nbPrint % 5) == 0) )
        	        {
                	        printf("\n");
				dirtyLine = 0;
                	}
			nbPrint++;
		}
		blockNum++;
	}
	if (dirtyLine)
	{
		printf("\n");
	}
	/* Zero at the end of the file, notify the user */
	if (hole_block != -1)
	{
		printf("Zero from 0x%llX to the end (File possibly corrupted)\n",
		((uint64_t)hole_block)*UBIFS_BLOCK_SIZE);

		hole_block = -1;
	}
	/* print the size found on the flash, in case of hole, size is leater then the file size */
	printf("Size on flash:%lld%s\n",
		len,
		(len == inoSize) ? "" : " (ERROR Ino Incoherent)");

	// Free TNC (free cache)
	shrinker_execute(c);
}

/**
 * Dump a directory
 * dent: dent node of the directory to print
 * parent_str: the parent path
 */
static void dump_directory(struct ubifs_info *c, struct ubifs_dent_node *dent, const char *parent_str)
{
	struct fscrypt_name nm = {0};
	union ubifs_key key;
	char str[STR_LEN];

	/* Generate a dent key */
	dent_key_init(c, &key, dent->inum, &nm);
	while (1)
	{
		/* In the first iteration, when nm is NULL,    the tnc function return entry of the key */
		/* In the other iteration, when nm is defined, the tnc function return the entry just after the key */
		dent = ubifs_tnc_next_ent(
      				c,
			  	&key,
				&nm);
		if (IS_ERR(dent))
		{
			break;
		}
		/* Generate a nice absolute pathname */
		sprintf(str, "%s%s%s", parent_str, FILE_SEP, dent->name);
		/* Print the name and the inode number */
		printf("\n%s:%s (inode=%lld)\n", ubifs_get_type_name(dent->type), str, dent->inum);

//		ubifs_dump_node(c, dent, dent->ch.len);

		switch (dent->type)
		{
			/* Case of a directory */
			case UBIFS_ITYPE_DIR:
			{
				/* Print the inode information */
				ads_print_ino_node(c, dent->inum);
				/* Recusive Call */
				dump_directory(c, dent, str);
			}
			break;
			/* Case of a Regular File */
			case UBIFS_ITYPE_REG:
			{
				/* Dump LEB of a file */
				dump_file(c, dent->inum);
			}
			break;

			default:
			{
				/* Other case: soft link, block device node, socket... */
				printf("No Dump Defined for %s\n", ubifs_get_type_name(dent->type));
			}
			break;
		}

		/* Re generate the key */
		key_read(c, &dent->key, &key);

		/* Set the dir or file name */
		fname_len(&nm) = le16_to_cpu(dent->nlen);
		fname_name(&nm) = (char *)dent->name;
		/* ubifs_tnc_next_ent alloc memory */
		kfree(dent);
	}
}


/**
 * Call dump_directory function from the root inode *
 */
void dump_fs_from_root(struct ubifs_info *c)
{
	union ubifs_key key;
	struct ubifs_dent_node *dent;

	dent = malloc(sizeof(*dent)+STR_LEN);

	/* Generate ino key of the ROOT Inode */
	ino_key_init(c, &key, UBIFS_ROOT_INO);

	if (ubifs_tnc_lookup(c, &key, dent))
	{
		printf("Error unable to find root node\n");
	}
	else
	{
		/* dump_directory function will add a slash */
		dump_directory(c, dent, "");
	}

	free(dent);
}

