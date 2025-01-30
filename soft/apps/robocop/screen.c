/************************ LIST OF SCREEN USERS ************************
 * Users who are running screen in the forground protect all the background
 * screen processes that they own.  This keeps a list of such users.
 */

#include <sys/types.h>
#include <sys/param.h>

#include "config.h"
#include "extern.h"

#ifdef PROTECT_SCREEN

/*
 * This is the hash table.  Key is user id number.  There is no data - a user
 * is a screener if his uid is in the hash.  Empty slots are represented by a
 * value of 0.  n_screeners gives the number of elements currently in the
 * hash.  The table's size is SCREENERSIZE, but we never put more than
 * MAXSCREENERS elements into it, MAXSCREENERS being 2/3 of SCREENERSIZE.
 * This is because linear probing becomes slow as too much of the table gets
 * full.
 */

#define SCREENERSIZE (int)(MAXSCREENERS*1.5)	/* Size of hash table */

uid_t           screener[SCREENERSIZE];
int             n_screener;

/*
 * INIT_SCREENER - set the screener table to the initial, no screeners state.
 */

void 
init_screener()
{
	int             i;
	n_screener = 0;
	for (i = 0; i < SCREENERSIZE; i++)
		screener[i] = 0;
}

/*
 * FIND_SCREENER - Return the index of the given uid in the hash table. This
 * does linear probing.  If the uid is not in the table, it returns a pointer
 * to the first empty slot in the table.  This is the place where you should
 * insert it if we want to do the insert.
 */

int 
find_screener(uid_t uid)
{
	int             i = hashfunc((char *) &uid, sizeof(uid_t), SCREENERSIZE);

	while (screener[i] != 0 && screener[i] != uid)
		if (++i >= SCREENERSIZE)
			i = 0;

	return i;
}


/*
 * IS_SCREENER - return true if the user is in the list of screeners. If the
 * hash table has overflowed, then we treat all users as screeners.
 */

int 
is_screener(uid_t uid)
{
	return (n_screener >= MAXSCREENERS || screener[find_screener(uid)] != 0);
}


/*
 * ADD_SCREENER - add the given uid to the list of screeners.
 */

void 
add_screener(uid_t uid)
{
	int             i;
	if (n_screener >= MAXSCREENERS)
		return;
	i = find_screener(uid);
	if (screener[i] == 0) {
		n_screener++;
		screener[i] = uid;
	}
}

#endif
