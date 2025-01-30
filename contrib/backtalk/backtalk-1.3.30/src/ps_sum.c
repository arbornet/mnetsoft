/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* PICOSPAN SUM FILE ROUTINES.
 *
 * These routines read PicoSpan sum files.  Only one sum file may be open
 * at a time.
 *
 *   opensum(dir)		Open the sum file for the conference stored
 * 				in the "conf_dir" system dictionary variable.
 *   last_no()			Return the number of the last item in the
 *				current conference.
 *   seek_item(item)		Position the file pointer to read the named
 *				item.  -1 seeks to last item.
 *   next_item(&item,&resp)	Find the next item number and the number
 *				of responses to it.
 *   prev_item(&item,&resp)	Find the previous item number and the number
 *				of responses to it.
 *   closesum()			Close the current sum file.
 *   
 *
 * Mar 15, 1993 - Jan Wolter:  Original bbsread version
 * Dec  2, 1995 - Jan Wolter:  Ansification
 * Jan  2, 1996 - Jan Wolter:  Backtalk version
 */

#include "backtalk.h"

#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
typedef struct dirent direntry;
#else
typedef struct direct direntry;
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include "sysdict.h"
#include "ps_part.h"
#include "ps_item.h"
#include "ps_post.h"
#include "ps_sum.h"
#include "lock.h"
#include "sum.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


int sum_fd= -1;
int sum_item;
char sum_path[BFSZ];
short sum_swap= 0;		/* Should sum-file data be byteswapped? */


int part_cksum(char *c);
void createsum(void);
int item_sume(int item, struct sume *sum, int newer_only);
int dir_max_item(void);


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
    default:
	    die("dunno who to swapbyte %d byte values",n);
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


/* OPENSUM -- Opens the sum file.
 */

void opensum(char *dir)
{
    struct sumh sh;

    if (sum_fd >= 0) return;

    sprintf(sum_path,"%s/sum",dir);

    if ((sum_fd= open(sum_path,O_RDWR)) < 0)
	createsum();

    /* Read and check magic code number */
    if (read(sum_fd,(char *)&sh,HEADSIZE) != HEADSIZE ||
	memcmp(sh.magic,SUM_MAGIC,8))
    {
	/* If sum file is invalid, blow it away and recreate it */
	unlink(sum_path);
	createsum();

	/* try again */
	if (read(sum_fd,(char *)&sh,HEADSIZE) != HEADSIZE ||
	    memcmp(sh.magic,SUM_MAGIC,8))
        	die("could not construct sane sum file");
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


/* SEEK_ITEM -- Position us at the given item of the sum file.  An item number
 * of -1 goes to the last item in the sum file.
 */

void seek_item(int item_no)
{
    opensum(sdict(VAR_CONFDIR));

    if (item_no == -1)
	sum_item= last_no();
    else if (item_no == 0)
	die("bad item number in seek_item");
    else
	sum_item= item_no;

    lseek(sum_fd, offset_of(sum_item),0);
}


/* LAST_NO -- Return the number of the last item in the sum file. */

int last_no()
{
    struct stat st;

    opensum(sdict(VAR_CONFDIR));
    fstat(sum_fd,&st);
    return items_in(st.st_size);
}


/* GET_SUM -- Return info from the sum file about an item.  Returns 1 if
 * the item has been killed, or 2 if no such item has been entered.  Resp_no
 * is the number of responses, including response 0.  If any of the return
 * parameters are passed in as NULL, no value is returned for those.
 */

int get_sum(int item, int *resp_no, int *flags,
	time_t *item_date, time_t *resp_date)
{
    struct sume sum;
    int rc, write_sum= 0;

    seek_item(item);
    if (read(sum_fd,(char *)&sum,sizeof(struct sume)) != sizeof(struct sume))
	return 2;
    if (sum_swap) swapsume(&sum);

    /* Can't trust sum file for linked items - must scan item if it is newer */
    if (sum.flags & SF_LINKED)
	write_sum= item_sume(sum_item, &sum, TRUE);

    if (rc= (sum.resp_no == 0 || sum.resp_date == 0L || sum.flags == 0))
	lost_items(item,item);
    else
    {
	if (resp_no != NULL) *resp_no= sum.resp_no;
	if (flags != NULL) *flags= sum.flags;
	if (item_date != NULL) *item_date= sum.item_date;
	if (resp_date != NULL) *resp_date= sum.resp_date;
    }

    if (write_sum)
    {
	seek_item(item);
	if (sum_swap) swapsume(&sum);
	write(sum_fd,(char *)&sum,sizeof(struct sume));
    }

    return rc;
}


/* PUT_SUM -- Update a sum file entry.  The item number must always be given.
 * sflags is the flags to turn on and rflags is the flags to turn off.
 * But if resp_no is -1 or either of the dates is 0L, then the old value for
 * that field will be retained.  If resp_no is -2, the number of responses is
 * incremented (the new response should already have been posted).  The new
 * number of responses is returned.
 *
 * This should only be called while you have a lock on the corresponding item
 * file.
 *
 */

int put_sum(int item, int resp_no, int sflags, int rflags,
	time_t item_date, time_t resp_date)
{
    struct sume sum;
    int rebuilt= 0;

    if (item < 1)
	die("bad item number in put_sum");
    
    lock_shared(sum_fd,sum_path);

    /* Get old values */
    if (resp_no == -1 || sflags == 0 || rflags == 0 || item_date == 0L ||
	resp_date == 0L)
    {
	seek_item(item);
	if (read(sum_fd,(char *)&sum,sizeof(struct sume)) !=
		sizeof(struct sume))
	    die("Attempt to update nonexistant sum file entry");
	if (sum_swap) swapsume(&sum);
	if (sum.flags & SF_LINKED)
        {
	    if (rebuilt= item_sume(sum_item, &sum, TRUE))
	    {
		seek_item(item);
		if (sum_swap) swapsume(&sum);
		write(sum_fd,(char *)&sum,sizeof(struct sume));
		if (sum_swap) swapsume(&sum);
	    }
        }
    }
    if (resp_no > -1)
	sum.resp_no= resp_no;
    else if (resp_no == -2 && !rebuilt)
	sum.resp_no++;		/* Don't do this if we just recounted resps */

    sum.flags&= ~rflags;
    sum.flags|= sflags;
    if (item_date != 0L) sum.item_date= item_date;
    if (resp_date != 0L) sum.resp_date= resp_date;

    seek_item(item);
    if (sum_swap) swapsume(&sum);
    if (write(sum_fd,(char *)&sum,sizeof(struct sume)) != sizeof(struct sume))
	die("write to sum file failed");

    unlock_active(sum_fd,sum_path);

    return sum.resp_no;
}


/* NEXT_ITEM -- Get the next real item in the sum file.  Return its number and
 * Number of responses.  If we hit the end of the file, we return 1.  Any
 * items which are missing, are marked for deletion from the participation
 * file.
 */

int next_item(int *item_no, int *resp_no, int *flags,
		time_t *item_date, time_t *resp_date)
{
    struct sume sum;
    int start_item= sum_item;

    opensum(sdict(VAR_CONFDIR));
    while (read(sum_fd,(char *)&sum,sizeof(struct sume)) == sizeof(struct sume))
    {
	if (sum_swap) swapsume(&sum);
	if (sum.flags & SF_LINKED)
	{
	    if (item_sume(sum_item, &sum, TRUE))
	    {
		lseek(sum_fd,offset_of(sum_item),0);
		if (sum_swap) swapsume(&sum);
		write(sum_fd,(char *)&sum,sizeof(struct sume));
		if (sum_swap) swapsume(&sum);
		lseek(sum_fd,0L,1);	/* fake seek between write and read */
	    }
	}
	if (sum.resp_no != 0 && sum.resp_date != 0L && sum.flags != 0)
	{
	    *item_no= sum_item;
	    *resp_no= sum.resp_no;
	    *flags= sum.flags;
	    *item_date= sum.item_date;
	    *resp_date= sum.resp_date;
	    if (sum_item != start_item)
		    lost_items(start_item,sum_item-1);
	    sum_item++;
	    return 0;
	}
	sum_item++;
	/*lseek(sum_fd,offset_of(sum_item),0);*/
    }
    if (sum_item != start_item)
	lost_items(start_item,-1);
    return 1;
}


/* PREV_ITEM -- Get the prev real item in the sum file.  Return it's number and
 * Number of responses.  If we hit the front of the file, we return 1.  Any
 * items which are missing, are marked for deletion in the participation file.
 */

int prev_item(int *item_no, int *resp_no, int *flags,
		time_t *item_date, time_t *resp_date)
{
    struct sume sum;
    int start_item= sum_item;

    opensum(sdict(VAR_CONFDIR));
    while (sum_item > 0)
    {
	if (read(sum_fd,(char *)&sum,sizeof(struct sume)) ==
		sizeof(struct sume))
	    die("strange error reading sumfile\n");
	if (sum_swap) swapsume(&sum);
	if (sum.flags & SF_LINKED)
	{
	    if (item_sume(sum_item, &sum, TRUE))
	    {
		lseek(sum_fd,offset_of(sum_item),0);
		if (sum_swap) swapsume(&sum);
		write(sum_fd,(char *)&sum,sizeof(struct sume));
		if (sum_swap) swapsume(&sum);
	    }
	}
	if (sum.resp_no != 0 && sum.resp_date != 0L && sum.flags != 0)
	{
	    *item_no= sum_item;
	    *resp_no= sum.resp_no;
	    *flags= sum.flags;
	    *item_date= sum.item_date;
	    *resp_date= sum.resp_date;
	    if (sum_item != start_item)
		lost_items(sum_item+1,start_item);
	    sum_item--;
	    if (sum_item > 0)
		lseek(sum_fd,offset_of(sum_item),0);
	    return 0;
	}
	sum_item--;
	lseek(sum_fd,offset_of(sum_item),0);
    }
    if (sum_item != start_item)
	lost_items(1,start_item);
    return 1;
}


/* PART_CHKSUM - Return the participation filename checksum for the current
 * conference.
 */

int part_cksum(char *c)
{
    int chksum= 0;

    for ( ; *c != '\0'; c++)
	chksum= (chksum << 2) ^ (*c);
    return chksum;
}


/* DIR_MAX_ITEM - Scan the current conference directory for the maximum
 * item number in the directory.
 */

int dir_max_item()
{
    DIR *dp;
    direntry *dir;
    int i, maxi= 0;

    if ((dp= opendir(sdict(VAR_CONFDIR))) == NULL)
	die("Cannot open directiory %s",sdict(VAR_CONFDIR));
    
    while ((dir= readdir(dp)) != NULL)
    {
	if (dir->d_name[0] == '_' && (i= atoi(dir->d_name+1)) > maxi)
		maxi= i;
    }
    closedir(dp);
    return maxi;
}


/* ITEM_SUME - Make a sum file entry for an existing item.  If "newer_only"
 * is set and an existing sum-file entry is passed in, then the sum-file
 * entry is reconstructed only if the modification date of the file is newer
 * than the response date in the old sum-file entry.
 *
 * Returns 1 if this is a rebuilt entry, 0 otherwise.
 */

int item_sume(int item, struct sume *sum, int newer_only)
{
    FILE *fp;
    struct stat st;
    char bf[BFSZ+1];

    /* Open the item file, with no lock on it */
    sprintf(bf,"%s/_%d",sdict(VAR_CONFDIR),item);
    if ((fp= fopen(bf,"r")) == NULL)
    {
	if (errno != ENOENT)
	    die("Could not open item file %s - %s",bf,strerror(errno));

	/* Item file does not exist -- blank sum file entry */
	sum->flags= 0;
	sum->resp_no= 0;
	sum->resp_date= 0;
	sum->item_date= 0;
	return 1;
    }
    open_item_index(sdict(VAR_CONFDIR),item);

    /* Stat the file */
    if (fstat(fileno(fp),&st))
	die("stat of %s failed",bf);
    
    if (newer_only && (sum->resp_date == st.st_mtime))
    {
	fclose(fp);
	close_item_index();
	return 0;
    }
    
    /* Set the flags */
    sum->flags= SF_ACTIVE;
    if (!(st.st_mode & 0200))
	sum->flags|= SF_FROZEN;
    if (st.st_mode & 0010)
	sum->flags|= SF_PARTY;
    if (st.st_mode & 0100 || !(st.st_mode & 0400))
	sum->flags|= SF_RETIRED;
    if (st.st_nlink > 1)
	sum->flags|= SF_LINKED;
    sum->resp_date= st.st_mtime;
    
#if 0
    /* Scan item file, counting responses and setting creation date */
    sum->item_date= 0;
    sum->resp_no= 0;
    while (fgets(bf, BFSZ, fp) != NULL)
	if (bf[0] == ',' && bf[1] == 'D')
	{
	    if (++sum->resp_no == 1 && !newer_only)
		sum->item_date= atoth(bf+2);
	}
#else

    /* Extract item date from first response (the item text) */
    if (!newer_only)
    {
	sum->item_date= 0;
	/* Read through file for first date line */
	while (fgets(bf, BFSZ, fp) != NULL)
	    if (bf[0] == ',' && bf[1] == 'D')
	    {
		sum->item_date= atoth(bf+2);
		break;
	    }
    }

    /* Count the number of responses in the item (using index to help) */
    sum->resp_no= seek_resp(fp,-1) + 1;

#endif
    fclose(fp);
    close_item_index();
    return 1;
}


/* CREATESUM - Make a sum file for a conference that hasn't got one.
 */

void createsum()
{
    struct sumh head;
    struct sume sum;
    int maxi, item;

    /* Create the file */
    if ((sum_fd= open(sum_path,O_RDWR|O_CREAT|O_EXCL,0644)) < 0)
    {
	if (errno == EEXIST)
	{
	    if ((sum_fd= open(sum_path,O_RDWR)) < 0)
		die("cannot open sum file (%s)",sum_path);
	    return;
	}
	else
	    die("cannot create sum file (%s)",sum_path);
    }

    lock_exclusive(sum_fd,sum_path);

    /* Set up and write the sum file header */
    strncpy(head.magic, SUM_MAGIC, 8);
    head.part_checksum= part_cksum(sdict(VAR_PARTICIP));
    head.dunno= DUNNO_VALUE;
    head.seq= SEQ_VALUE;
    write(sum_fd, (char *)&head, sizeof(struct sumh));

    /* Count the items in the conference */
    maxi= dir_max_item();

    for (item= 1; item <= maxi; item++)
    {
	item_sume(item, &sum, FALSE);
	write(sum_fd, (char *)&sum, sizeof(struct sume));
    }

    unlock_active(sum_fd,sum_path);

    lseek(sum_fd,0L,0);	/* rewind */
}


/* LINK_SUM:  Modify the sum files in the two conference directories named to
 * indicate that the dst_item is a link to the src_item.
 */

void link_sum(char *src_dir, int src_item, char *dst_dir, int dst_item)
{
    struct sume sum;

    /* Goto Source conference */
    closesum();
    opensum(src_dir);

    /* Read original sum file entry */
    seek_item(src_item);
    if (read(sum_fd,(char *)&sum,sizeof(struct sume)) != sizeof(struct sume))
	die("can't read item %d from %s sumfile", src_item, src_dir);
    if (sum_swap) swapsume(&sum);

    /* Ensure linked flag is set in original entry */
    if (!(sum.flags & SF_LINKED))
    {
	sum.flags|= SF_LINKED;

	/* Write back modified sum-file entry */
	seek_item(src_item);
	if (sum_swap) swapsume(&sum);
	if (write(sum_fd,(char *)&sum,sizeof(struct sume)) !=
		sizeof(struct sume))
	    die("can't write item %d to %s sumfile", src_item, src_dir);
    }
    closesum();

    /* Write sum file entry to destination conference sumfile */

    sum.item_date= time(0L);

    opensum(dst_dir);
    seek_item(dst_item);
    if (sum_swap) swapsume(&sum);
    if (write(sum_fd,(char *)&sum,sizeof(struct sume)) !=
	    sizeof(struct sume))
	die("can't write item %d to %s sumfile", dst_item, dst_dir);
    closesum();
}
