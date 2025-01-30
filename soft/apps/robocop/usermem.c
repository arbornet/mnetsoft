/**********************  ADD UP USER MEMORY ********************/

#include "config.h"
#include "extern.h"

/*
 * Hash table for user information.  Table size is actually half again as
 * many users as MAXUSERS is defined as, but efficiency is poor if it gets
 * near full. so we never put more than MAXUSERS users into it.  We use
 * linear probing here.  Empty slots are indicated by an nproc value of 0.
 */

#define TABLESIZE (int)(1.5*MAXUSERS)
struct userhash {
	uid_t           uid;	/* key - user's uid */
	unsigned long   mem;	/* total pages of memory used */
	int             nproc;	/* number of processes begin used */
}               user[TABLESIZE];
int             n_user = 0;


/* INIT_USERMEM - initialize the user hash table to an empty state.  */

void 
init_usermem()
{
	int             i;

	n_user = 0;
	for (i = 0; i < TABLESIZE; i++)
		user[i].nproc = 0;
}


/*
 * ADD_USERMEM - add the given value to the amount of memory being used by
 * the user with the given uid.  Also increment the number of processes. If
 * the table is full (really 2/3 full) then we still update users already in
 * the hash, but don't add more.
 */

void 
add_usermem(uid_t uid, unsigned long mem)
{
	int             i, j;

	/* Hash into table, then probe till we find it or an empty slot */
	i = j = uid % TABLESIZE;
	while (user[i].uid != uid && user[i].nproc > 0) {
		if (++i >= TABLESIZE)
			i = 0;
		/* Just give up if the table is full - should never happen */
		if (i == j)
			return;
	}

	if (user[i].nproc == 0) {
		if (n_user >= MAXUSERS)
			return;

		/* Create a new entry */
		n_user++;
		user[i].uid = uid;
		user[i].mem = mem;
		user[i].nproc = 1;
	} else {
		/* Update an old entry */
		user[i].mem += mem;
		user[i].nproc++;
	}
}


/*
 * CHECK_MEM - Find users who are using too much memory, or too many
 * processes, and murder them.
 */

void 
check_mem(kvm_t * kd)
{
	int             i, j, mem, kill;

	/* Loop through hash table, looking for violaters to prosecute */
	for (i = 0; i < TABLESIZE; i++) {
		/* Skip empty slots */
		if (user[i].nproc == 0)
			continue;
#ifdef VERBOSE
		if (logfp)
			fprintf(logfp, "CHECK_MEM uid=%d mem=%ldK n=%d\n",
			user[i].uid, user[i].mem * kperpage, user[i].nproc);
#endif
		if ((mem = user[i].mem * pagesize) >= LOG_MEMORY ||
		    user[i].nproc >= LOG_NPROC) {
			/* Make sure it isn't a protected uid */
			for (j = 0; protect_uids[j] != -1; j++)
				if (user[i].uid == protect_uids[j])
					goto next_user;

			/* Should we kill him, or just log him? */
			kill = (mem >= MAX_MEMORY || user[i].nproc >= MAX_NPROC);

			/* Log the kill */
			if (logfp) {
#ifdef LOG_USER_NAMES
				struct passwd  *pw = getpwuid(user[i].uid);
				if (pw != NULL)
					fprintf(logfp,
						"process limit %s by user %s.  n=%d mem=%ld\n",
					   kill ? "exceeded" : "approached",
					   pw->pw_name, user[i].nproc, mem);
				else
#endif
					fprintf(logfp,
						"process limit %s for uid %d.  n=%d mem=%ld\n",
					   kill ? "exceeded" : "approached",
					   user[i].uid, user[i].nproc, mem);
			}
			/* Do the kill */
			if (kill)
				killuser(kd, user[i].uid, logfp);
			else
				loguser(kd, user[i].uid, logfp);
		}
next_user:	;
	}
#ifdef VERBOSE
	if (logfp)
		fflush(logfp);
#endif
}
