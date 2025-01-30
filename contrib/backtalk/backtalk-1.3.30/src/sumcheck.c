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

#include "sum.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int sum_fd= -1;
short sum_swap= 0;		/* Should sum-file data be byteswapped? */

/* Flags */
int ignore_idate= 0;
int ignore_rdate= 0;
int ignore_rnum= 0;
int ignore_flags= 0;
int repair= 0;
int create= 0;
int silent= 0;


/* ATOTH -- This interprets a character string as a hex string and returns an
 * time_t equivalent.
 */

time_t atoth(char *str)
{
    time_t n=0;

    for( ; isascii(*str) && isxdigit(*str); str++)
            n= n*16 + *str - (isdigit(*str) ? '0' 
                                            : (isupper(*str) ? 'A' : 'a') - 10);    return n;
}


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


/* PART_CHKSUM - Return the participation filename checksum for the the given
 * participation file name.
 */

int part_cksum(char *p)
{
    int chksum= 0;

    while (*p != '\0')
        chksum= (chksum << 2) ^ (*(p++));
    return chksum;
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

/* GETPFNAME - Get the participation file name for the conference in the
 * given directory.  Returned value is in static memory.
 */

char *getpfname(char *dir_path)
{
    static char bf[BFSZ+1];
    char *p;
    FILE *fp;

    strncpy(bf,dir_path,BFSZ-10);
    strcat(bf,"/config");
    if ((fp= fopen(bf,"r")) == NULL)
    {
    	printf("Cannot open conference config file %s\n",bf);
	exit(1);
    }
    if (fgets(bf, BFSZ, fp) == NULL || strcmp(bf,"!<pc02>\n") ||
	fgets(bf, BFSZ, fp) == NULL)
    {
    	printf("Bad config file\n");
	exit(1);
    }

    if ((p= strchr(bf,'\n')) != NULL) *p= '\0';

    return bf;
}


/* OPENSUM -- Opens the sum file.
 */

void opensum(char *sum_path, char *dir_path)
{
    struct sumh sh;

    if (sum_fd >= 0)
    	closesum();

    if ((sum_fd= open(sum_path, repair ? O_RDWR : O_RDONLY)) < 0)
    {
	if (!create)
	{
	    printf("cannot open sum file %s\n",sum_path);
	    exit(1);
	}
	if ((sum_fd= open(sum_path,O_CREAT|O_RDWR|O_EXCL,0644)) < 0)
	{
	    printf("cannot create sum file %s\n",sum_path);
	    exit(1);
	}

	/* Construct and write the header */
	strncpy(sh.magic, SUM_MAGIC, 8);
	sh.part_checksum= part_cksum(getpfname(dir_path));
	sh.dunno= DUNNO_VALUE;
	sh.seq= SEQ_VALUE;
	write(sum_fd, (char *)&sh, sizeof(struct sumh));

	/* Prepare for upcoming attempt to read */
	lseek(sum_fd, 0L, 1);
	return;
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


/* DIR_MAX_ITEM - Scan the current conference directory for the maximum
 * item number in the directory.
 */

int dir_max_item(char *confdir)
{
    DIR *dp;
    direntry *dir;
    int i, maxi= 0;

    if ((dp= opendir(confdir)) == NULL)
    {
	printf("Cannot open directiory %s\n",confdir);
	exit(1);
    }

    while ((dir= readdir(dp)) != NULL)
    {
	if (dir->d_name[0] == '_' && (i= atoi(dir->d_name+1)) > maxi)
                maxi= i;
    }
    closedir(dp);
    return maxi;
}

checkfile(char *file, int item, struct sume *sum)
{
    FILE *fp;
    char bf[BFSZ+1];
    struct sume new;
    struct stat st;
    int saveall;

    if ((fp= fopen(file,"r")) == NULL)
    {
        new.flags= 0;
	new.resp_no= 0;
	new.resp_date= 0;
	new.item_date= 0;
    }
    else
    {
    	if (fstat(fileno(fp),&st))
	{
	    printf("Cannot stat file %s",file);
	    exit(1);
	}
	new.flags= SF_ACTIVE;
	if (!(st.st_mode & 0200))
	    new.flags|= SF_FROZEN;
	if (st.st_mode & 0010)
	    new.flags|= SF_PARTY;
	if (st.st_mode & 0100 || !(st.st_mode & 0400))
	    new.flags|= SF_RETIRED;
	if (st.st_nlink > 1)
	    new.flags|= SF_LINKED;
	new.resp_date= st.st_mtime;

	/* Read through file to count responses and get first date line */
	new.item_date= 0;
	new.resp_no= 0;
	while (fgets(bf, BFSZ, fp) != NULL)
	    if (bf[0] == ',' && bf[1] == 'D')
	    {
		if (++new.resp_no == 1)
		    new.item_date= atoth(bf+2);
	    }

        fclose(fp);
    }

    if ((new.resp_no == sum->resp_no || ignore_rnum) &&
        (new.flags == sum->flags || ignore_flags) &&
        (new.resp_date == sum->resp_date || ignore_rdate) &&
	(new.item_date == sum->item_date || ignore_idate))
	    return 0;

    printf("Item %d:",item);
    if (new.flags == 0)
    {
	printf(" Vanished\n");
	saveall= 1;
    }
    else if (sum->flags == 0)
    {
    	printf(" Added\n");
	saveall= 1;
    }
    else
    {
	if (!ignore_rnum && new.resp_no != sum->resp_no)
	{
	    printf(" Resp %d -> %d", sum->resp_no, new.resp_no);
	}
	if (!ignore_flags && new.flags != sum->flags)
	{
	    printf(" Flags %08X -> %08X", sum->flags, new.flags);
	}
	if (!ignore_rdate && new.resp_date != sum->resp_date)
	{
	    printf(" RDate %24.24s", ctime(&sum->resp_date));
	    printf(" -> %24.24s", ctime(&new.resp_date));
	}
	if (!ignore_idate && new.item_date != sum->item_date)
	{
	    printf(" IDate %24.24s", ctime(&sum->item_date));
	    printf(" -> %24.24s", ctime(&new.item_date));
	}
    	printf("\n");
    }

    if (saveall || !ignore_rnum) sum->resp_no= new.resp_no;
    if (saveall || !ignore_flags) sum->flags= new.flags;
    if (saveall || !ignore_rdate) sum->resp_date= new.resp_date;
    if (saveall || !ignore_idate) sum->item_date= new.item_date;
    return 1;
}


main(int argc, char **argv)
{
    int i,j,max,rc= 0;
    struct sume sum;
    char *confdir= NULL, *sumfile= NULL, *itemfile;

    /* Parse command line arguments */
    for (i= 1; i < argc; i++)
    {
	if (argv[i][0] == '-')
	{
	    for (j= 1; argv[i][j] != '\0'; j++)
		switch (argv[i][j])
		{
		case 'i':
		    ignore_idate= 1;
		    break;
		case 'r':
		    ignore_rdate= 1;
		    break;
		case 'n':
		    ignore_rnum= 1;
		    break;
		case 'f':
		    ignore_flags= 1;
		    break;
		case 'c':
		    create= 1;
		    /* drops through */
		case 'u':
		    repair= 1;
		    break;
		case 's':
		    silent= 1;
		    break;
		default:
		    goto usage;
		}
	}
	else if (confdir == NULL)
	{
	    confdir= argv[i];
	}
	else if (sumfile == NULL)
	{
	    sumfile= argv[i];
	}
	else
	    goto usage;
    }
    if (confdir == NULL)
	goto usage;

    /* Construct sumfile name if it was not given */
    i= strlen(confdir);
    if (sumfile == NULL)
    {
	sumfile= (char *)malloc(i+6);
	strcpy(sumfile,confdir);
	strcpy(sumfile+i,"/sum");
    }

    /* Construct root of item file name */
    itemfile= (char *)malloc(i+15);
    strcpy(itemfile,confdir);
    strcpy(itemfile+i,"/_");
    j= i + 2;

    opensum(sumfile,confdir);

    max= dir_max_item(confdir);

    for (i= 1; ; i++)
    {
	if (read(sum_fd,(char *)&sum,sizeof(struct sume)) !=
	    sizeof(struct sume))
	    break;

	if (sum_swap) swapsume(&sum);

	sprintf(itemfile+j, "%d", i);
	if (checkfile(itemfile, i, &sum))
	{
	    rc= 1;
	    if (repair)
	    {
		/* Back up over last entry read */
		if (lseek(sum_fd, offset_of(i), 0) < 0)
		{
		    printf("Seek failed\n");
		    exit(1);
		}
		/* Write new value */
		if (sum_swap) swapsume(&sum);
		write(sum_fd, &sum, sizeof(struct sume));
		/* Seek before reading again */
		lseek(sum_fd, offset_of(i+1), 0);
	    }
	}
    }

    while (i <= max)
    {
	sprintf(itemfile+j, "%d", i);
	sum.resp_no= 0;
	sum.flags= 0;
	sum.resp_date= 0L;
	sum.item_date= 0L;
	if (checkfile(itemfile, i, &sum))
	{
	    rc= 1;
	    if (repair)
	    {
		lseek(sum_fd, offset_of(i), 0);
		if (sum_swap) swapsume(&sum);
		write(sum_fd, &sum, sizeof(struct sume));
	    }
	}
	i++;
    }

    exit(rc);

usage:
    printf("usage: %s -[cusnfri] <confdir> [<sumfile>]\n",argv[0]);
    printf("  -u   Update. Repair any errors found.\n");
    printf("  -c   Create. Create a new sum file if none found.  Imples -u.\n");
    printf("  -s   Silent. Do not report errors found.\n");
    printf("  -n   Number. Do not report or repair response count errors.\n");
    printf("  -f   Flag. Do not report or repair item flag errors.\n");
    printf("  -r   Response. Do not report or repair response date errors.\n");
    printf("  -i   Item. Do not report or repair item date errors.\n");
    exit(1);
}
