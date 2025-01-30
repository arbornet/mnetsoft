/*
 * $Id: isshellok.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <sys/types.h>

#include <pwd.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

/**
 * @name isshellvalid
 *
 * @param shell String pointing to the full pathname of the shell to test.
 * @return Return 1 if shell appears in /etc/shells, 0 otherwise.
 * @see getusershell(3)
 * @memo Return true of the passed shell is in /etc/shells.
 */
int
isshellvalid(char *shell)
{
	char *sh;

	setusershell();
	while ((sh = getusershell()) != NULL)
		if (STREQ(sh, shell))
			break;
	endusershell();

	return (sh != NULL);
}

/**
 * @name isshellok
 *
 * @param uid User ID number of user to test shell of.
 * @return Return 1 if shell appears in /etc/shells, 0 otherwise.
 * @see getusershell(3)
 * @memo Return true if the shell used by the specified uid is in /etc/shells.
 */
int
isshellok(int uid)
{
	struct passwd *pw;

	pw = getpwuid(uid);
	if (pw == NULL)
		return(0);

	return(isshellvalid(pw->pw_shell));
}
