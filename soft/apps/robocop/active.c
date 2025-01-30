/************************ LIST OF LOGGED IN TTYS ************************
 * These routines maintain a database of which tty's have a user logged in
 * to them, and which may thus have processes running on them.  In this
 * version we just load them into a hash.  If it is in the hash, it is
 * active, otherwise it is not.
 */

#include <sys/types.h>
#include <sys/param.h>

#include "config.h"
#include "extern.h"


/*
 * This is the hash table.  Key is device number.  There is not data - a tty
 * is active it it's device number is in the hash.  Empty slots are
 * represented by a value of NODEV.  n_active gives the number of elements
 * currently in the hash.  The table's size is ACTIVESIZE, but we never put
 * more than ACTIVEMAX elements into it, ACTIVEMAX being 2/3 of ACTIVESIZE.
 * This is because linear probing becomes slow as too much of the table gets
 * full.
 */

#define ACTIVEMAX  MAXUSERS	/* Max number of logged in ttys */
#define ACTIVESIZE (int)(ACTIVEMAX*1.5)	/* Size of hash table */

dev_t           active[ACTIVESIZE];
int             n_active;

/*
 * INIT_ACTIVE - set the tty table to an initial no-users-logged state. This
 * can be called to initiallize the table before the first use, or to reset
 * it to the initial state.
 */

void 
init_active()
{
	int             i;
	n_active = 0;
	for (i = 0; i < ACTIVESIZE; i++)
		active[i] = NODEV;
}


/*
 * HASHFUNC - A hash function.  Pretty generic.  Could be used with strings
 * just by passing strlen(s) in as the string length, or with random hunks of
 * memory by typecasting their pointers to char *.
 */

int 
hashfunc(char *s, int len, int tblsize)
{
	unsigned int    hash;
	hash = *s++;
	while (--len > 0) {
		hash = ((hash << 2) | (hash >> 30)) ^ *s++;
	}
	return (int) (hash % tblsize);
}


/*
 * FIND_ACTIVE - Return the index of the given device in the hash table. This
 * does linear probing.  If the device is not in the table, it returns a
 * pointer to the first empty slot in the table.  This is the place where you
 * should insert it if we want to do the insert.
 */

int 
find_active(dev_t device)
{
	int             i = hashfunc((char *) &device, sizeof(dev_t), ACTIVESIZE);

	while (active[i] != NODEV && active[i] != device)
		if (++i >= ACTIVESIZE)
			i = 0;

	return i;
}


/*
 * GET_ACTIVE - return true if the device is in the list of logged in
 * terminals.  If the hash table has overflowed, then we treat all terminals
 * as logged in.
 */

int 
get_active(dev_t device)
{
	return (n_active >= ACTIVEMAX || active[find_active(device)] != NODEV);
}


/*
 * SET_ACTIVE - set the status of the given tty to active.  If the hash table
 * is full (actually 2/3 full), we do nothing.
 */

void 
set_active(dev_t device)
{
	int             i;
	if (n_active >= ACTIVEMAX)
		return;
	i = find_active(device);
	if (active[i] == NODEV) {
		n_active++;
		active[i] = device;
	}
}
