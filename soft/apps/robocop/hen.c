/************************ LIST OF HEN PROCESSES ************************
 * Some protected processes protect their child processes.  We call these
 * hens and keep a list of them.  Well, actually we keep a hash of them.
 */

#include <sys/types.h>
#include <sys/param.h>

#include "config.h"
#include "extern.h"


/*
 * This is the hash table.  Key is process number.  There is no data - a
 * process is a hen if it's pid is in the hash.  Empty slots are represented
 * by a value of 0.  n_hens gives the number of elements currently in the
 * hash.  The table's size is HENSIZE, but we never put more than HENMAX
 * elements into it, HENMAX being 2/3 of HENSIZE.  This is because linear
 * probing becomes slow as too much of the table gets full.
 */

#define HENMAX  MAXPROCS	/* Max number of protected procs */
#define HENSIZE (int)(HENMAX*1.5)	/* Size of hash table */

pid_t           hen[HENSIZE];
int             n_hen;

/*
 * INIT_HEN - set the hen table to the initial, no hens state.
 */

void 
init_hen()
{
	int             i;
	n_hen = 0;
	for (i = 0; i < HENSIZE; i++)
		hen[i] = 0;
}

/*
 * FIND_HEN - Return the index of the given pid in the hash table. This does
 * linear probing.  If the pid is not in the table, it returns a pointer to
 * the first empty slot in the table.  This is the place where you should
 * insert it if we want to do the insert.
 */

int 
find_hen(pid_t pid)
{
	int             i = hashfunc((char *) &pid, sizeof(pid_t), HENSIZE);

	while (hen[i] != 0 && hen[i] != pid)
		if (++i >= HENSIZE)
			i = 0;

	return i;
}


/*
 * IS_HEN - return true if the process is in the list of hen processes. If
 * the hash table has overflowed, then we treat all processes as hens.
 */

int 
is_hen(pid_t pid)
{
	return (n_hen >= HENMAX || hen[find_hen(pid)] != 0);
}


/*
 * ADD_HEN - add the given pid to the list of hens.
 */

void 
add_hen(pid_t pid)
{
	int             i;
	if (n_hen >= HENMAX)
		return;
	i = find_hen(pid);
	if (hen[i] == 0) {
		n_hen++;
		hen[i] = pid;
	}
}
