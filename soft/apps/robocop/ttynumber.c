/******************** FIND DEVICE NUMBERS FOR TTY NAMES *******************
 * We basically do this by stating the /dev/ttyXX device, however we
 * typically need to do this on the same terminals repeatedly, so for the
 * sake of efficiency, we cache the information in a hash.
 */

#include "config.h"
#include "extern.h"

#include <utmp.h>
#include <sys/param.h>


/* The hash table */

#define TTYNUMBERMAX MAXTTYS
#define TTYNUMBERSIZE (int)(TTYNUMBERMAX*1.5)

struct ttynonode {
	char            name[UT_LINESIZE + 1];	/* Name of tty - null if slot
						 * is empty */
	dev_t           device;	/* device number for tty */
}               ttynumber[TTYNUMBERSIZE];

int             n_ttynumber;	/* Number of entries in the hash table */


/*
 * INIT_TTYNUMBER() - routine to initialize the hash table.  Should be called
 * once.
 */

void 
init_ttynumber()
{
	int             i;
	n_ttynumber = 0;
	for (i = 0; i < TTYNUMBERSIZE; i++)
		ttynumber[i].name[0] = '\0';
}


/*
 * FIND_TTYNUMBER() - Find a given entry in the hash table.  We use linear
 * probing.  If the entry is not found, we return a pointer to the empty slot
 * where it should be stored.
 */

int 
find_ttynumber(char *tty)
{
	int             len, i;

	/* Get length of tty name, which might not be null terminated */
	for (len = 0; len < UT_LINESIZE && tty[len] != '\0'; len++);

	for (i = hashfunc(tty, len, TTYNUMBERSIZE);
	     ttynumber[i].name[0] != '\0' &&
	     strncmp(ttynumber[i].name, tty, UT_LINESIZE);
	     i = (i + 1) % TTYNUMBERSIZE);
	return i;
}


/*
 * GET_TTYNUMBER - lookup a tty in the hash table, and return it's device
 * number.  If it is not in the hash table, return NODEV.
 */

dev_t 
get_ttynumber(char *tty)
{
	int             i = find_ttynumber(tty);
	if (ttynumber[i].name[0] == '\0')
		return NODEV;
	return ttynumber[i].device;
}


/*
 * SET_TTYNUMBER - add a tty/device number pair to the hash table.  If the
 * table is already 2/3 full, we silently decline to store it.  Such overflow
 * devices will always have to be statted.
 */

void 
set_ttynumber(char *tty, dev_t device)
{
	int             i;
	if (n_ttynumber >= TTYNUMBERMAX)
		return;
	i = find_ttynumber(tty);
	if (ttynumber[i].name[0] == '\0') {
		n_ttynumber++;
		strlcpy(ttynumber[i].name, tty, UT_LINESIZE + 1);
	}
	ttynumber[i].device = device;
}


/*
 * GETDEVNUM - Return the device number for a "ttyXX" or "console" style
 * device name.  Device name does not have to be null terminated, so stuff
 * from the utmp file can be passwd right in.  This will try first to get the
 * value from the hash table, and if it fails, stat the device (storing the
 * result in the hash table as well as returning it). Dies on failure.
 */

dev_t 
getdevnum(char *tty)
{
	char            fullname[31];
	struct stat     st;
	dev_t           dev;

	/* First try our hash table */
	if ((dev = get_ttynumber(tty)) != NODEV)
		return dev;

	/* Otherwise, stat the device */
	snprintf(fullname, sizeof(fullname), "/dev/%.*s", UT_LINESIZE, tty);
	if (stat(fullname, &st)) {
		if (logfp)
			fprintf(logfp, "ERROR: could not stat %s", fullname);
		exit(1);
	}
	/* Add it to the hash */
	set_ttynumber(tty, st.st_rdev);

	return st.st_rdev;
}
