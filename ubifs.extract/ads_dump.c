/**
 * AIRBUS DEFENCE & SPACE
 * October 2025
 * F. FRAYSSE
 */


#include "linux_err.h"

#include "ads_dump.h"


/* LEB Number to dump in cas it was specifed by arg */
static int lebToDump = -1;


/* Extension name to find in cas we want to extract file */
#define EXTENSION_TO_DUMP (".json")

/* MTD device to use to find PEB <-> LEB association */
#define MTD_DEVICE ("/dev/mtd1")

/* The nanddump script filename */
#define NANDUMP_FILENAME ("/home/root/nand_dump.shell")

/* Helper to know sizeo fo an array */
#define NB_ELEM_OF(x) (sizeof(x)/sizeof(x[0]))

/* String separator for printf */
#define THE_SEPARATOR ("########\n")


/* Number of file to dump in the static array fileToDumpList */
static int nbFileToDump = 0;


/* File list to dump LEB and extract */
static struct
{
	struct ubifs_dent_node *node;
}
fileToDumpList[50];


/* malloc allocated size for a data node */
#define DATA_NODE_SIZE (64536)
/* Point to a malloc area to store data_node (size is DATA_NODE_SIZE) */
static struct ubifs_data_node *data_node = NULL;

/* Empty page: usefull when a data entry not found */
static uint8_t *emptyBlock = NULL;

/* File descriptor for the shell file to generate extracted file, out of this sw with nanddump */
static FILE *fdShell = NULL;




/**
 *  Add a new file to the static list of file to dump
 */
static void addFileToDump(struct ubifs_dent_node *dent)
{
	/* Limit the number of file's */
	if (nbFileToDump >= NB_ELEM_OF(fileToDumpList))
	{
		printf("Too many file to dump !!\n");
		return;
	}
	/* Add a file to dump, and nupdate the number of file */
	fileToDumpList[nbFileToDump].node = dent;
	nbFileToDump++;
}

/**
 * Check is the filename is a file to dump
 */
static int isFileToDump(char *name)
{
	unsigned int extLen;

	extLen = strlen(EXTENSION_TO_DUMP);

	/*First if the name to check is not too short */
	if (strlen(name) < extLen)
	{
		return 0;
	}

	/* Forward name to the end of the string */
        name += strlen(name);

	/* Backward the name to the start of a possible filename extansion */
	name -= strlen(EXTENSION_TO_DUMP);

	/* Time to check */
        return 0 == strcmp(name, EXTENSION_TO_DUMP);
}

/**
 * Print information of a LEB
 *
 * block: only for printf, to identify the block number
 * data_node: the data node to locate
 * pnum: To store the Physical Erasable Block location
 * lnum: To store the logical Erasable Block Location
 * pebOffs: Offset of data's relative to a PEB
 * lebOffs: Offset of data's relative of a LEB
 * size: only for printf, size of thhe data containing in this node
 */
static void printLEB(
		struct ubifs_info *c,
	       	int block,
	       	struct ubifs_data_node *data_node,
		int *pnum,
		int *lnum,
		int *pebOffs,
		int *lebOffs,
		int size)
{
	struct ubifs_ch *ch;
	union ubifs_key key;
	int err;
	
	/* Generate the key to looking for */
	key_read(c, &data_node->key, &key);
	
	/* Alloc area for the function ubifs_tnc_locate */
	ch = malloc(data_node->ch.len);

	/* Use the lib ubifs function to obtain the node + LEB + LEB offset */
	/* In case of success return 0 */
	err = ubifs_tnc_locate(c, &key, ch, lnum, lebOffs);
	if (err != 0)
	{
		/* Notify user of the error, usually ENOENT */
		printf("Error ubifs_tnc_locate block=%d err=%d (%s)\n",
			block,
			err,
			strerror(err));
		(*lnum)    = -1;
		(*pebOffs) = -1;
		(*lebOffs) = -1;
	}
	else
	{
		/* Using PEB LEB helper to known the physical PEB */
                (*pnum )   = peb_leb_getPeb(*lnum);
	
		/* Compute the PEB offset */
		(*pebOffs) = 0;
		if ( (*pnum) >= 0)
		{
			(*pebOffs) = peb_leb_getDataOffset(*pnum) + (*lebOffs);
		}

		/* offset is data node (header + data), keep in mind the header */
		printf(
				"block #%d %s PEB %d:%d LEB %d:%d size:%d\n",
				block,
				dbg_ntype(ch->node_type),
				(*pnum),
				(*pebOffs),
				(*lnum),
				(*lebOffs),
				size);
	}
	/* Return the allocated area */
	free(ch);
}

/**
 * Print information about the inode
 *
 * inode: inode number to print
 * Return the size field.
 */
uint64_t ads_print_ino_node(struct ubifs_info *c, uint64_t inode)
{
	uint64_t inoSize = ~0;
	struct ubifs_ino_node *pInode;
	union ubifs_key key;
	char modeStr[100];
	int lnum;
	int offs;
	int err;

	/* Alloc more space */
	pInode = malloc(8192+sizeof(struct ubifs_ino_node));

	/* Generate an inode key */	
	ino_key_init(c, &key, inode);

	/* Use lib ubifs to find the key */
	err = ubifs_tnc_lookup(c, &key, pInode);
	if (0 == err)
	{
		/* Generate a nice sightseeing */
		sprintf(
				modeStr,
				"mode:0x%X (%c%c%c%c%c%c%c%c%c)",
				pInode->mode,
				 // user
				 (pInode->mode&(1<<8))?'r':'-',
				 (pInode->mode&(1<<7))?'w':'-',
				 (pInode->mode&(1<<6))?'x':'-',
				 // group
				 (pInode->mode&(1<<5))?'r':'-',
                                 (pInode->mode&(1<<4))?'w':'-',
                                 (pInode->mode&(1<<3))?'x':'-',
				 // other
				 (pInode->mode&(1<<2))?'r':'-',
                                 (pInode->mode&(1<<1))?'w':'-',
                                 (pInode->mode&(1<<0))?'x':'-');

		printf("ino_inode: size:%lld %s uid=%d gid=%d\n",
				pInode->size,
				modeStr,
				pInode->uid,
				pInode->gid);

		inoSize = pInode->size;
	}
	else
	{
		/* Error case */
		printf("%s: Unable to find:%lld err=%d (%s)\n",
			__FUNCTION__,
			inode,
			err,
			strerror(err));
	}

	/* Re generate a key to locate the LEB (code from linux driver) */
	key_read(c, &pInode->key, &key);
	err = ubifs_tnc_locate(c, &key, pInode, &lnum, &offs);
        if (err != 0)
	{
		/* Error not found */
		printf("Error ubifs_tnc_locate ino_inode err=%d (%s)\n", err, strerror(err));
	}
	else
	{
		/* Print the ino inode location */
		printf("ino_inode: LEB:%d:%d, PEB:%d\n",lnum, offs, peb_leb_getPeb(lnum));
	}
	// Return memory (for help: man free) */
	free(pInode);

	return inoSize;
}

/**
 * Generate the file to extract in /home/root/
 * Generate a nanddump script
 */
static void extract_file(struct ubifs_info *c, struct ubifs_dent_node *node)
{
	char outFile[400];
	char command[500];
	union ubifs_key key;
	int          err;
	int          block = 0;
	unsigned int partSize;
	uint64_t     extractedSize = 0;
	FILE        *fd;
	int          lnum;
        int          pebOffs;
	int          lebOffs;
	int          pnum;
	uint64_t     fileSize;
	uint64_t     leftSize;

	fileSize = ads_print_ino_node(c, node->inum);
	printf("Extract file:%s size:%lld\n", node->name, fileSize);

	/* Open the file to extract */
	sprintf(outFile, "/home/root/%s", node->name);
	fd = fopen(outFile, "w");
	if (fd == NULL)
	{
		printf("Unable to open file to write\n");
		return;
	}

	/* Alloc a static variable if not allocated */
	if (data_node == NULL)
	{
		data_node = malloc(DATA_NODE_SIZE);
	}
	/* Alloc zero block */
	if (emptyBlock == NULL)
	{
		
		emptyBlock = calloc(1, UBIFS_BLOCK_SIZE);
	}

	/* Extract all the file */
	while (extractedSize < fileSize)
	{
		/*  Generate a data key */
		data_key_init(c, &key, node->inum, block);
		/* Use the lib ubifs function to lookup */
		err = ubifs_tnc_lookup(c, &key, data_node);

		if (err == -ENOENT)
		{
			/* NO ENTRY: page with zero */
			if (fd != NULL)
			{
				leftSize = fileSize - extractedSize;
				if (1 != fwrite(
					emptyBlock,
					(leftSize>=UBIFS_BLOCK_SIZE) ? UBIFS_BLOCK_SIZE : leftSize,
					1,
					fd))
				{
					printf("Unable to write file\n");
					fclose(fd);
					fd = NULL;
				}
			}
			extractedSize += UBIFS_BLOCK_SIZE;
		}
		else if (err)
		{
			printf("ubifs_tnc_lookup err:0x%X (%s)\n", err, strerror(err));
			extractedSize += UBIFS_BLOCK_SIZE;
		}
		else
		{
			/* Compute the size of the data */
			partSize = le32_to_cpu(data_node->ch.len) - UBIFS_DATA_NODE_SZ;
			extractedSize += partSize;

			if (fd)
			{
				if (1 != fwrite(data_node->data,  partSize, 1, fd))
				{
					printf("Unable to write file\n");
					fclose(fd);
					fd = NULL;
				}
			}

			pnum = -1;
			printLEB(c, block, data_node, &pnum, &lnum, &pebOffs, &lebOffs, partSize);

			if ( (pnum >= 0) && (fdShell != NULL) )
			{
				fprintf(
						fdShell,
					       	"nanddump /dev/mtd1 -s 0x%llX -l %d | tail -c %d | dd bs=%d count=1 >> /home/root/%s.nanddDmp\n",
						(uint64_t)peb_leb_get_eb_size()*pnum,
						peb_leb_get_eb_size(),
						peb_leb_get_eb_size()-pebOffs-offsetof(struct ubifs_data_node,data),
						partSize,
						node->name);
			}
		}

		block++;
	}

	if (fd != NULL)
	{
		fclose(fd);
		fd = NULL;
	}

	sprintf(command, "md5sum %s", outFile);
	printf("exe cmd:%s\n", command);
	fflush(stdout);
	system(command);
}

/**
 *
 *
 */
static void extract_files_list(struct ubifs_info *c)
{
	int i;
	char command[500];

	fdShell = fopen(NANDUMP_FILENAME, "w");
	if (fdShell == NULL)
	{
		printf("Unable to open for write %s script\n", NANDUMP_FILENAME);
		return;
	}

	/* For each file's to extract */
	for (i=0; i<nbFileToDump; i++)
	{
		printf("\n");
		extract_file(c, fileToDumpList[i].node);
	}

	if (fdShell != NULL)
	{
		fprintf(fdShell, "md5sum /home/root/*.json /home/root/*Dmp | sort\n");
		fclose(fdShell);
		fdShell = NULL;
	}

	printf(THE_SEPARATOR);
	sprintf(command, "cat %s", NANDUMP_FILENAME);
	printf("%s\n", command);
	fflush(stdout);
	system(command);
}

/**
 * Print each file of a directory
 */
static void print_sub_dir(struct ubifs_info *c, struct ubifs_dent_node *dent)
{
	struct fscrypt_name nm = {0};
	union ubifs_key key;
	int lnum;
	int offs;
	struct ubifs_ch *ch;
	int toDump;

	/* Generate a dentry key */
	dent_key_init(c, &key, dent->inum, &nm);

	while (1)
	{
		dent = ubifs_tnc_next_ent(
      				c,
			  	&key,
				&nm);
		if (IS_ERR(dent))
		{
			break;
		}

		key_read(c, &dent->key, &key);

		ch = malloc(dent->ch.len);

		if (ubifs_tnc_locate(c, &key, ch, &lnum, &offs))
		{
			printf("Error ubifs_tnc_locate\n");
		}
		toDump = isFileToDump((char *)dent->name);
		printf("ubifs_dent_node inum:%lld name:\"%s\" lnum:%d offs:%d toDump:%d\n",
				 dent->inum,
				 (char *)dent->name,
				 lnum,
				 offs,
				 toDump);
		if (toDump)
		{
			addFileToDump(dent);
		}

		fname_len(&nm) = le16_to_cpu(dent->nlen);
		fname_name(&nm) = (char *)dent->name;
	}
}

/**
 * Print the file's contained in the dir
 */
static int print_dir(struct ubifs_info *c, const char* the_dir)
{
	struct ubifs_inode *root_ui;
	struct ubifs_dent_node *dent;
	struct fscrypt_name nm = {0};
	union ubifs_key key;
	int err = 0;

	printf("\nprint_dir:\"%s\" ", the_dir);

	root_ui = ubifs_lookup_by_inum(c, UBIFS_ROOT_INO);
	if (IS_ERR(root_ui))
       	{
		err = PTR_ERR(root_ui);
		/* Previous step ensures that the root dir is valid. */
		ubifs_assert(c, err != -ENOENT);
		return err;
	}

	if (root_ui->flags & UBIFS_CRYPT_FL)
       	{
		ubifs_msg(c, "The root dir is encrypted, skip checking lost+found");
		return -1;
	}

	fname_name(&nm) = the_dir;
	fname_len(&nm) = strlen(the_dir);

	dent = kmalloc(UBIFS_MAX_DENT_NODE_SZ, GFP_NOFS);
	if (!dent)
	{
		return (-ENOMEM);
	}

	dent_key_init(c, &key, root_ui->vfs_inode.inum, &nm);
	err = ubifs_tnc_lookup_nm(c, &key, dent, &nm);
	if (!err)
       	{
		printf("inum=%lld name=\"%s\"\n", dent->inum, dent->name);
		print_sub_dir(c, dent);
	}
	else
	{
		printf(" => Not found !\n");

	}

	return err;
}

/**
 * Set the LEB to dump
 */
void ads_set_leb_to_dump(int leb)
{
	lebToDump = leb;
}

/**
 * Main function
 */
void ads_dump( struct ubifs_info *c)
{
	int err;

	/* Dump a LEB, using program arg */
	if (lebToDump >= 0)
	{
		ubifs_dump_leb(c, lebToDump);
		return;
	}

        printf(THE_SEPARATOR);
        fflush(stdout);

	/* Look up LEB<->PEB association */
	err = peb_leb_init(MTD_DEVICE);
	if (err)
	{
		printf("Error peb_leb_init return:%d\n", err);
		return;
	}
        fflush(stdout);

	/* Print the directory content */
	/* Saves file's with .json extension */
	printf(THE_SEPARATOR);
	print_dir(c, "ci");
	print_dir(c, "je_nexiste_pas");
      	print_dir(c, "tables");
	printf(THE_SEPARATOR);
	fflush(stdout);

	/* dump the saved file */
	extract_files_list(c);

	printf(THE_SEPARATOR);
	printf("Dumping all file from root\n");
        fflush(stdout);

	dump_fs_from_root(c);
	
        printf(THE_SEPARATOR);
        fflush(stdout);

	return;
}

