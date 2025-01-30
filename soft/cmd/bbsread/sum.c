/*
 * SUM file read routines.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbsread.h"


/* Structure of a sum file entry */
struct sume {
	unsigned int flags;	/* Flags */
	unsigned int resp_no;	/* Number of responses on item */
	long resp_date;		/* Date of last response */
	long item_date;		/* Date item linked into conf */
};

#define HEADSIZE 20

/* Offset of item n in the sum file */
#define offset_of(n) (sizeof(struct sume)*(long)(n-1) + HEADSIZE)

/* Number of items in a sum file of size n */
#define items_in(n) ( (n-2*sizeof(struct sume)+HEADSIZE)/sizeof(struct sume) )

FILE *sum_fp;
int sum_item;

int last_no();



/*
 * OPEN_SUM -- Opens the sum file.  It returns 1 if there is no sum file.
 */

int
open_sum(char *dir)
{
	char buf[BFSZ];

	snprintf(buf, sizeof buf, "%s/sum", dir);

	if ((sum_fp = fopen(buf, "r")) == NULL)
		return (1);

	/* Read and check magic code number */
	if (fgets(buf, BFSZ, sum_fp) == NULL || strcmp(buf, "!<sm02>\n")) {
		fprintf(stderr, "%s: conference sum file in weird format\n",
			progname);
		exit(1);
	}
	return (0);
}


/*
 * SEEK_ITEM -- Position us at the given item of the sum file.  An item
 * number of -1 goes to the last item in the sum file.
 */

void
seek_item(int item_no)
{
	if (item_no == -1)
		sum_item = last_no();
	else
		sum_item = item_no;

	fseek(sum_fp, offset_of(sum_item), 0);
}


/* LAST_NO -- Return the number of the last item in the sum file. */

int
last_no()
{
	struct stat st;

	fstat(fileno(sum_fp), &st);
	return (items_in(st.st_size));
}


/*
 * NEXT_ITEM -- Get the next real item in the sum file.  Return it's number
 * and Number of responses.  If we hit the end of the file, we return 1.  Any
 * items which are missing, are marked for deletion in the participation
 * file.
 */

int
next_item(int *item_no, int *resp_no)
{
	struct sume sum;
	int start_item = sum_item;

	while (fread(&sum, sizeof(struct sume), 1, sum_fp) != 0) {
		if (sum.resp_no != 0 || sum.resp_date != 0L) {
			*item_no = sum_item;
			*resp_no = sum.resp_no;
			if (sum_item != start_item)
				lost_items(start_item, sum_item - 1);
			sum_item++;
			/* fseek(sum_fp,offset_of(sum_item),0); */
			return (0);
		}
		sum_item++;
		/* fseek(sum_fp,offset_of(sum_item),0); */
	}
	if (sum_item != start_item)
		lost_items(start_item, -1);
	return (1);
}


/*
 * PREV_ITEM -- Get the prev real item in the sum file.  Return it's number
 * and Number of responses.  If we hit the front of the file, we return 1.
 * Any items which are missing, are marked for deletion in the participation
 * file.
 */

int
prev_item(int *item_no, int *resp_no)
{
	struct sume sum;
	int start_item = sum_item;

	while (sum_item > 0) {
		if (fread(&sum, sizeof(struct sume), 1, sum_fp) == 0) {
			fprintf(stderr, "%s: strange error reading sumfile\n",
				progname);
			exit(1);
		}
		if (sum.resp_no != 0 || sum.resp_date != 0L) {
			*item_no = sum_item;
			*resp_no = sum.resp_no;
			if (sum_item != start_item)
				lost_items(sum_item + 1, start_item);
			sum_item--;
			if (sum_item > 0)
				fseek(sum_fp, offset_of(sum_item), 0);
			return (0);
		}
		sum_item--;
		fseek(sum_fp, offset_of(sum_item), 0);
	}
	if (sum_item != start_item)
		lost_items(1, start_item);
	return (1);
}

void
close_sum()
{
	fclose(sum_fp);
}
