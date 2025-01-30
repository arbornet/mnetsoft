/*
 * PARTIPATION FILE reader.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

#include <ctype.h>
#include <pwd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bbsread.h"


char alias[BFSZ];		/* Users alias in the cf */
struct pfe {
	int item_no;		/* Item number */
	int resp_no;		/* Number of responses read including item */
	int forgotten;		/* Has this item been forgotten? */
	int old_resp_no;	/* Original value of resp_no */
	long date;		/* Time of last reading */
	int delete;		/* Should this entry not be written out? */
	struct pfe *next;	/* Pointer to next entry */
} *pflist;

char filename[BFSZ] = "";	/* Full pathname of participation file */
bool no_pf = true;		/* Don't use a participation file */

struct passwd *getpwuid();
void fullpfpath();


/*
 * PFREAD -- Read in a user's participation file.  Return 1 if there isn't
 * one.
 */

void
weird_pf()
{
	fprintf(stderr, "%s: participation file %s in weird format\n",
		progname, filename);
	exit(1);
}


int
pfread(char *name)
{
	FILE *fp;
	char buf[BFSZ], *t;
	struct pfe *npf, *lpf = NULL;

	fullpfpath(name);

	if ((fp = fopen(filename, "r")) == NULL) {
		snprintf(filename, sizeof filename, "%s/%s", getenv("HOME"), name);
		if ((fp = fopen(filename, "r")) == NULL)
			return (1);
	}
	/* Read and check magic code number */
	if (fgets(buf, BFSZ, fp) == NULL || strcmp(buf, "!<pr03>\n"))
		weird_pf();

	/* Read alias name */
	if (fgets(alias, BFSZ, fp) == NULL)
		weird_pf();
	*(firstin(alias, "\n")) = '\0';


	while (fgets(buf, BFSZ, fp) != NULL) {
		npf = (struct pfe *) malloc(sizeof(struct pfe));
#ifdef YAPP
		if (sscanf(buf, "%d %d %lx", &npf->item_no,
			   &npf->resp_no,
			   (unsigned long *) &npf->date) != 3)
			weird_pf();
		if (npf->forgotten = (npf->resp_no < 0))
			npf->resp_no = -npf->resp_no
#else
		/* Fancier scanning to handle case of "-0" responses */
		if (!isascii(buf[0]) || !isdigit(buf[0]))
			weird_pf();
		npf->item_no = atoi(buf);
		t = firstout(firstin(buf, " "), " ");
		if ((npf->forgotten = (*t == '-')) != 0)
			t++;
		if (sscanf(t, "%d %lx", &npf->resp_no,
			   (unsigned long *) &npf->date) != 2)
			weird_pf();
#endif				/* YAPP */
		npf->old_resp_no = npf->resp_no;
		npf->delete = 0;
		npf->next = NULL;
		if (lpf == NULL)
			pflist = npf;
		else
			lpf->next = npf;
		lpf = npf;
	}

	fclose(fp);
	no_pf = false;
	return (0);
}

/*
 * PFNEW -- Create a new participation file for the conference.
 */

void
pfnew(char *name)
{
	struct passwd *pw;

	fullpfpath(name);

	pflist = NULL;

	/* Set alias name */
	pw = getpwuid(getuid());
	strncpy(alias, pw->pw_gecos, BFSZ);

	no_pf = false;
	return;
}


/*
 * LAST_READ -- Return the number of the last response to item that the user
 * has read.  Return resp_no = -1 if it is unread.  The flag forgotten is
 * true if the item has been forgotten.
 */

void
last_read(int item, int *resp_no, bool * forgotten)
{
	struct pfe *pf;

	*forgotten = false;
	*resp_no = -1;

	if (no_pf)
		return;

	for (pf = pflist; pf != NULL; pf = pf->next) {
		/* Item not in participation file...is new */
		if (pf->item_no > item)
			return;

		/* If we found it, return the response number */
		if (pf->item_no == item) {
			*forgotten = pf->forgotten;
			*resp_no = pf->resp_no - 1;
			return;
		}
	}
	return;
}


/*
 * UPDATE_ITEM -- Mark an item read up through the given response.  If the
 * response is -1, forget the item (or remember it if it is forgotten).
 */

void
update_item(int item, int resp)
{
	struct pfe *pf, *pfp, *pfn;

	if (no_pf)
		return;

	for (pf = pflist, pfp = NULL; pf != NULL; pfp = pf, pf = pf->next) {
		if (pf->item_no > item)
			break;

		if (pf->item_no == item) {
			if (resp == -1)
				pf->forgotten = !pf->forgotten;
			else {
				pf->resp_no = resp + 1;
				pf->date = time(0L);
			}
			return;
		}
	}

	/* Item not in participation file...put it in */
	pfn = (struct pfe *) malloc(sizeof(struct pfe));
	pfn->item_no = item;
	if (resp == -1) {
#ifdef YAPP
		pfn->resp_no = 1;	/* Can't forget unread item */
#else
		pfn->resp_no = 0;
#endif				/* YAPP */
		pfn->forgotten = 1;
		pfn->date = 0;
	} else {
		pfn->resp_no = resp + 1;
		pfn->forgotten = 0;
		pfn->date = time(0L);
	}
	pfn->old_resp_no = 0;
	pfn->delete = 0;
	pfn->next = pf;
	if (pfp == NULL)
		pflist = pfn;
	else
		pfp->next = pfn;
	return;
}


/*
 * RESET_ITEM -- Reset the resp_no to it's original value.
 */

void
reset_item(int item)
{
	struct pfe *pf;

	if (no_pf)
		return;

	for (pf = pflist; pf != NULL; pf = pf->next) {
		if (pf->item_no > item)
			break;

		if (pf->item_no == item) {
			pf->resp_no = pf->old_resp_no;
			break;
		}
	}
}


/*
 * LOST_ITEMS -- The items between first and last are missing.  Delete them
 * from the partipation file.  Really just mark them for deletion.  If last
 * is -1, then delete through the end of the conference.
 */
void
lost_items(int first, int last)
{
	struct pfe *pf;
	int state = 1;

	if (no_pf || last < first)
		return;

	if (last == -1)
		last = INT_MAX;

	for (pf = pflist; pf != NULL; pf = pf->next) {
		if (state == 1 && pf->item_no >= first)
			state = 2;

		if (state == 2) {
			if (pf->item_no > last)
				break;
			pf->delete = 1;
		}
	}
}


/*
 * PFWRITE -- Write out the participation file.  A pfread() call must preceed
 * this.
 */

void
pfwrite()
{
	FILE *fp;
	struct pfe *pf, *npf;

	if (no_pf)
		return;

	if ((fp = fopen(filename, "w+")) == NULL) {
		fprintf(stderr, "%s: cannot open %s to write\n",
			progname, filename);
		exit(1);
	}
	fputs("!<pr03>\n", fp);
	fputs(alias, fp);
	fputc('\n', fp);

	for (pf = pflist; pf != NULL; pf = npf) {
		if (!pf->delete) {
#ifdef YAPP
			fprintf(fp, "%d %d %lX\n",
				pf->item_no,
				pf->resp_no * (pf->forgotten ? 1 : -1),
				(unsigned long) pf->date);
#else
			/* Picospan style -0 for forgotten unseen items */
			fprintf(fp,
			     pf->forgotten ? "%d -%d %lX\n" : "%d %d %lX\n",
				pf->item_no,
				pf->resp_no,
				(unsigned long) pf->date);
#endif				/* YAPP */
		}
		npf = pf->next;
		free(pf);
	}
	fclose(fp);
}


/*
 * FULLPFPATH -- Construct the full path name of the participation file,
 * given the file name.
 */

void
fullpfpath(char *name)
{
	char *s;

	snprintf(filename, sizeof filename, "%s/.cfdir", getenv("HOME"));
	s = strrchr(filename, '/');
	if (access(filename, 0))
		*s = '\0';
	strlcat(filename, "/",  sizeof filename);
	strlcat(filename, name, sizeof filename);
}
