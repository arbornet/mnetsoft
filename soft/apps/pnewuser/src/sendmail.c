/*
 * Wrapper used on grex.org; sendmail invokes a maildrop
 * binary that only works for people in a "validated" group.
 * Newuser is not in that group (for what hopefully should
 * be obvious reasons) so instead invoke this program.  It
 * should be installed mode ---x--s--- owned by
 * newuser/$group (where $group is the group that can invoke
 * sendmail).  Then change the path to sendmail in newuser.
 */

#include <sys/types.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s command [args]\n", argv[0]);
		return(EXIT_FAILURE);
	}
	setgid(getegid());
	execv("/usr/sbin/sendmail", argv);
	perror("execv");
	return(EXIT_FAILURE);
}
