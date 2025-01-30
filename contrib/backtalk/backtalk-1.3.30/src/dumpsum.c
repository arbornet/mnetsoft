/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "sum.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int sum_fd= -1;
short sum_swap= 0;		/* Should sum-file data be byteswapped? */


/* SWAPBYTES - This reverses the byte order of an n-byte integer */

void swapbytes(int *val, int n)
{
    unsigned char t;
#define BYTE(i) (((unsigned char *)val)[i])

    switch(n)
    {
    case 1: break;
    case 2: t= BYTE(0); BYTE(0)= BYTE(1); BYTE(1)= t;
	    break;
    case 4: t= BYTE(0); BYTE(0)= BYTE(3); BYTE(3)= t;
	    t= BYTE(1); BYTE(1)= BYTE(2); BYTE(2)= t;
	    break;
    }
}


/* SWAPSUME - This reverses the byte order of everything in a sum entry */

void swapsume(struct sume *sum)
{
    swapbytes((int *)&(sum->flags), sizeof(sum->flags));
    swapbytes((int *)&(sum->resp_no), sizeof(sum->resp_no));
    swapbytes((int *)&(sum->resp_date), sizeof(sum->resp_date));
    swapbytes((int *)&(sum->item_date), sizeof(sum->item_date));
}


/* CLOSE_SUM -- Close the sum file.
 */

void closesum()
{
    if (sum_fd >= 0)
    {
	close(sum_fd);
	sum_fd= -1;
    }
}


/* OPENSUM -- Opens the sum file.
 */

void opensum(char *sum_path)
{
    struct sumh sh;

    if (sum_fd >= 0)
    	closesum();

    if ((sum_fd= open(sum_path,O_RDONLY)) < 0)
    {
	printf("cannot open sum file in %s\n",sum_path);
	exit(1);
    }

    /* Read and check magic code number */
    if (read(sum_fd,(char *)&sh,HEADSIZE) != HEADSIZE ||
	memcmp(sh.magic,SUM_MAGIC,8))
    {
	    printf("conference sum file in weird format\n");
	    exit(1);
    }

#ifdef OLDYAPP
    /* Swap the sum-file if we are on a little-endian machine */
    sum_swap= 1;
    sum_swap= (*((char *)&sum_swap));
#else
    /* Swap the sum-file if the sequence comes out backwards */
    sum_swap= (sh.seq == SEQ_BACKWARDS);
#endif
}


main(int argc, char **argv)
{
    int i;
    struct sume sum;

    if (argc != 2)
    {
	printf("usage: %s <sumfile>\n",argv[0]);
	exit(1);
    }

    opensum(argv[1]);

    for (i= 1; ; i++)
    {
	if (read(sum_fd,(char *)&sum,sizeof(struct sume)) !=
	    sizeof(struct sume))
	    break;

	if (sum_swap) swapsume(&sum);

	printf("%3d: resp=%3d flag=%8x rdate=%8x idate=%8x\n",i,
	    sum.resp_no, sum.flags, sum.resp_date, sum.item_date);
    }
}
