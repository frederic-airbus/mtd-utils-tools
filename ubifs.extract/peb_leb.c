/**
 * AIRBUS DEFENCE & SPACE
 * October 2025
 * F. FRAYSSE
 */


#include "ads_dump.h"

#include <mtd/ubi-media.h>
#include <mtd/mtd-user.h>
#include <sys/ioctl.h>
#include <libmtd.h>


#define NB_ELEM_OF(x) (sizeof(x)/sizeof(x[0]))

/* uncomment to print all PEB table */
//#define PEB_LEB_SHOW


/* Store ingormation about the mtd device */
static struct mtd_dev_info mtd;


/* Value of lnum in the list */
enum E_LNUM_VALUE
{
	LNUM_NOT_INIT = -1,
	LNUM_ERROR    = -2,
	LNUM_BAD      = -3,
	LNUM_ERASED   = -4,
};

static struct
{
	/* enum E_LNUM_VALUE or the associated LEB */
	int lnum;

	int data_offset;
}
/* Contain all the PEB */
*pebList = NULL;

/* Number of PEB for this MTD device */
static int pebNumber = -1;


/**
 * Return the PEB associated with a LEB
 * Not optimized: Better to have a sorted LEB list and use qsearch
 * So it do jobs
 */
int peb_leb_getPeb(int leb)
{
	int found = -1;
	int i;

	for (i=0; i<pebNumber; i++)
	{
		if (pebList[i].lnum == leb)
		{
			found = i;
			break;
		}
	}
	return found;
}

/**
 * Return the first data LEB inside the PEB
 */
int peb_leb_getDataOffset(int peb)
{
	if (pebList[peb].lnum >= 0)
	{
		return pebList[peb].data_offset;
	}
	else
	{
		printf("%s: PEB %d with no LEB\n", __FUNCTION__, peb);
		return 0;
	}
}

/**
 * Return the erasable size of the MTD device
 */
int peb_leb_get_eb_size(void)
{
	return mtd.eb_size;
}


/**
 * Read LEB information for the specified PEB index
 * So, the PEB may be bad or not mapped
 * crc of header not checked
 */
static int peb_leb_read_lnum(const int fd, const int idx, const int pebSize)
{
	off64_t           offset;
	struct ubi_ec_hdr  ech;
	struct ubi_vid_hdr vidh;
	int                lnum;

	/* Compute the offset of first data of the desired PEB */
	offset  = pebSize;
	offset *= idx;

	/* First consider ERROR */
	pebList[idx].lnum = LNUM_ERROR;

	/* Go to the first data of the PEB */
	if ( offset != lseek64(fd, offset, SEEK_SET))
	{
		printf("%s:%d Error\n", __FILE__, __LINE__);
		return -1;
	}

	/* Read the erasable counter header */
	if (sizeof(ech) != read(fd, &ech, sizeof(ech)))
	{
		printf("%s:%d Error\n", __FILE__, __LINE__);
                return -1;
	}

	/* Check presence of the magic */
	if ( __builtin_bswap32(ech.magic) != UBI_EC_HDR_MAGIC)
	{
 		printf("%s:%d Error\n", __FILE__, __LINE__);
                return -1;
	}

	/* Forward offset to the vid header of the PEB (vid:Volume IDentifier) */
	offset += __builtin_bswap32(ech.vid_hdr_offset);

	/* Go to the VID Hdr */
	if ( offset != lseek64(fd, offset, SEEK_SET))
        {
                printf("%s:%d Error\n", __FILE__, __LINE__);
                return -1;
        }

	/* Read the VID HDR */
        if (sizeof(vidh) != read(fd, &vidh, sizeof(vidh)))
        {
                printf("%s:%d Error\n", __FILE__, __LINE__);
                return -1;
        }

	/* Check the VID HDR magic number */
        if ( __builtin_bswap32(vidh.magic) != UBI_VID_HDR_MAGIC)
        {
		/* Bad magic */
		/* More check... */
		if (vidh.magic == 0xFFFFFFFF)
		{
			/* It's an Erased unmap PEB */
			pebList[idx].lnum = LNUM_ERASED;
			return 0;
		}
		else
		{
			/* Don't known what it is, error */
			printf("%s:%d Error\n", __FILE__, __LINE__);
			return -1;
		}
        }

	/* Convert to machine endianess */
	lnum = __builtin_bswap32(vidh.lnum);

	/* Robustness check: Out of range check */
	if ( (lnum <0) && (lnum > pebNumber) )
	{
		printf("%s:%d Error lnum:0x%X\n", __FILE__, __LINE__, lnum);
                return -1;
	}
	/* lnum is correct, save it */
	pebList[idx].lnum        = lnum;
	/* Convert to machine endianess the offset of the data's */
	/* So point the first data of the LEB */
	pebList[idx].data_offset = __builtin_bswap32(ech.data_offset);

	return 0;
}

/**
 * Check if there is one LEB for one PEB.
 * Fast coded: Not optimised jobs, may be better to have a LEB sorted list
 * So it do the jobs and called once
 */
static void peb_leb_check(void)
{
        int i, j;

	printf("%s: (Note: LEB0 superblock, LEB1 master, each on 2 PEB, redundacy)\n", __FUNCTION__);
        /* Check lnum are unique */
        for (i=0; i<pebNumber; i++)
        {
		/* May be PEB is a bad block */
		if (pebList[i].lnum == LNUM_BAD)
		{
			printf("PEB %d is BAD BLK\n", i);
		}

		/* Second loop: on last iteration of the first loop, due to j+1, loop not executed */
                for (j=i+1; j<pebNumber; j++)
                {
                        if (
					(pebList[i].lnum == pebList[j].lnum) &&
					(pebList[i].lnum >= 0) &&
					(pebList[j].lnum >= 0) )

                        {
                                 printf("%s:%d Error Multiple LEB %d PEB1 %d PEB2 %d\n",
						 __FILE__,
						 __LINE__,
                                                 pebList[j].lnum,
                                                 i,
                                                 j);
                        }
                }
        }
}


#ifdef PEB_LEB_SHOW

/**
 * Show the PEB table
 */
static void peb_leb_show(void)
{
	int i;


	printf("Link PEB => LEB:");
	for (i=0; i<pebNumber; i++)
	{
		if (  (i % 5) ==0)
                {
                        printf("\n");
                }

		switch (pebList[i].lnum)
		{
			case LNUM_NOT_INIT:
				printf("%4d:INIT|", i);
				break;
			case LNUM_ERROR:
				printf("%4d:ERRO|", i);
				break;
			case LNUM_BAD:
				printf("%4d:BAD |", i);
 				break;
			case LNUM_ERASED:
				printf("%4d:UMAP|", i);
                                break;
			default:
				printf("%4d:%4d|", i, pebList[i].lnum);
				break;
		}		
	}
	printf("\n");
}
#endif

/**
 * Initialize the PEB/LEB information
 * use ioctl MEMGETINFO
 */
int peb_leb_init(const char *mtd_device)
{
	libmtd_t mtd_desc;
	int fd;
	int i;


	if (pebList == NULL)
	{
		/* Initialize libmtd */
		mtd_desc = libmtd_open();
		if (!mtd_desc)
		{
			printf("can't initialize libmtd\n");
			return __LINE__;
		}

		/* Open the device in read only mode */
		printf("Opening %s in RDONLY\n", mtd_device);
 		fd = open(mtd_device, O_RDONLY);
                if (fd < 0)
                {
                        printf("%s: Unable to open %s\n", __FUNCTION__, mtd_device);
                        return __LINE__;
                }

		/* Fill in MTD device capability structure */
		if (mtd_get_dev_info(mtd_desc, mtd_device, &mtd) < 0)
		{
			printf("%s: Error mtd_get_dev_info\n", __FUNCTION__);
                        return __LINE__;
		}

		/* Save the erasable counter */
		pebNumber = mtd.eb_cnt;

		printf("Total Size:%lld\n", mtd.size);
		printf("Erase Size:%d\n",   mtd.eb_size);
		printf("Write Size:%d\n",   mtd.min_io_size);
		printf("Number of PEB:%d\n", pebNumber);


		/* Allocate area with zero */
		pebList = calloc(pebNumber, sizeof(*pebList));

		/* Initialise all PEB with an invalid LEB information */
		for (i=0; i<pebNumber; i++)
		{
			pebList[i].lnum = LNUM_NOT_INIT;
		}

		printf("Reading PEB LEB link\n");
		for (i=0; i<pebNumber; i++)
		{
			/* Before accessing to the PEB, check if it's BAD */
			if (mtd_is_bad(&mtd, fd, i))
			{
//				printf("PEB #%d is Bad\n", i);
				pebList[i].lnum = LNUM_BAD;
			}
			else if(peb_leb_read_lnum(fd, i, mtd.eb_size))
			{
				printf("Error PEB %d\n", i);
			}
		}

		peb_leb_check();
#ifdef PEB_LEB_SHOW
		peb_leb_show();
#endif

		close(fd);

		printf("end\n");
	}
	else
	{
		printf("%s: already initialized\n", __FUNCTION__);
	}
	return 0;
}


