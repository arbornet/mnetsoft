/*
 * DUMMY TEST VERSION OF GETMAILFILE ROUTINE.  Get rid of this when we have
 * hierarchical mail done.
 */

#include <pwd.h>
#include <stdio.h>

/*
 * GETMAILFILENAME Give a user's pw file entry, and a format string (ignored
 * in this dummy version) return the full path to their mailbox file.
 */

int
getmailfilename(struct passwd * pw, char *fmt, char *bf, int sz)
{
	snprintf(bf, sz, "/var/mail/%s", pw->pw_name);
	return 1;
}
