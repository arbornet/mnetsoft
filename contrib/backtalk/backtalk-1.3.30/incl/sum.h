/* Structure of a sum file header */
struct sumh {
	char magic[8];			/* "!<sm02>\n"  version string */
	int part_checksum;		/* Checksum on particpation file name */
	int dunno;			/* usually 0x00001537 */
	unsigned long seq;		/* 0x12345678 */
	};
#define HEADSIZE sizeof(struct sumh)
#define DUNNO_VALUE	0x1537
#define SEQ_VALUE	0x12345678
#define SEQ_BACKWARDS	0x78563412
#define SUM_MAGIC	"!<sm02>\n"

/* Structure of a sum file entry */
struct sume {
	unsigned int flags;		/* Flags */
	unsigned int resp_no;		/* Number of responses on item */
	time_t resp_date;		/* Date of last response */
	time_t item_date;		/* Date item linked into conf */
	};

/* Offset of item n in the sum file */
#define offset_of(n) (sizeof(struct sume)*(long)((n)-1) + HEADSIZE)

/* Number of items in a sum file of size n */
#define items_in(n) (((n)-2*sizeof(struct sume)+HEADSIZE)/sizeof(struct sume) )

#ifndef SF_RETIRED

/* Sum File Flags */
#define SF_RETIRED 0x0002               /* Retired item */
#define SF_DUNNO   0x000c
#define SF_ACTIVE  0x0030               /* Active item */
#define SF_FROZEN  0x0040               /* Frozen item */
#define SF_PARTY   0x0080               /* Party item */
#define SF_LINKED  0x0100               /* Linked item */

#endif
