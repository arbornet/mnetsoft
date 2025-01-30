/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* /etc/shadow External Authentication Database Interface
 *
 * This module supports authentication out of a unix shadow password file
 * using mod_auth_external and pwauth, but could readily be adapted to use
 * any other external authentication module.
 */


#include "common.h"
#include "authident.h"
#include "waittype.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_auth_module= "auth_shadow";

/* AUTH_GETPASS - If we could actually read the password, then we wouldn't be
 * stupid enough to use an external authenticator, so we can assume that
 * passwords are not available.
 */

#ifdef HAVEAUTH_GETPASS
char *auth_getpass(char *login)
{
    return "";
}
#endif


/* CHECKPASS - Given a login and password, return TRUE if they are are
 * correct.  Runs an external authenticator.
 */

#ifdef HAVEAUTH_CHECKPASS
int auth_checkpass(char *login, char *passwd)
{
    pid_t child_pid, wait_pid;
    waittype wrc;
    int pip[2];

    if (pipe(pip))
	return 0;

    switch(child_pid = fork())
    {
    case 0: /* Child - exec the command */

	close(pip[1]);

	/* Attach pip[0] to child's stdin */
	if (pip[0] != 0)
	{
	    dup2(pip[0],0);
	    close(pip[0]);
	}

	execl(PWAUTH_PATH, (char *)NULL);
	/* Exec failed */ exit(-1);

    case -1: /* Parent - Fork failed */

	close(pip[0]);
	close(pip[1]);

	return 0;

    default: /* Parent - Fork succeeded */

	close(pip[0]);

	/* Send the data down the pipe */
	write(pip[1], login, strlen(login));
	write(pip[1], "\n", 1);
	write(pip[1], passwd, strlen(passwd));
	write(pip[1], "\n", 1);

	close(pip[1]);

	/* Wait for child process to exit */
	do {
	    if ((wait_pid = wait(&wrc)) == -1)
		return 0;
	} while (wait_pid != child_pid);

	/* If child was killed, deny authentication */
	if (!WIFEXITED(wrc))
	    return 0;

	/* Child exited cleanly, return status */
	
	return !WEXITSTATUS(wrc);
    }
}
#endif
