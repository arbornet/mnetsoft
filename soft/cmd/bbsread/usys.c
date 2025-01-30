#include <sys/wait.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bbsread.h"

void execcmd();

/*
 * UPOPEN - Run command on a pipe
 *
 * This is similar to the Unix popen() and pclose() calls, except upopen() runs
 * the command with the original user id.
 */

FILE *f_lastpop = NULL;		/* Current upopened stream (NULL means none ) */
int p_lastpop;			/* Process ID of current upopened child */

FILE *
upopen(char *cmd, char *mode)
{
	int pip[2];
	register int chd_pipe, par_pipe;
	FILE *fdopen();

	/* Make a pipe */
	if (pipe(pip))
		return ((FILE *) 0);

	/* Choose ends */
	par_pipe = (*mode == 'r') ? pip[0] : pip[1];
	chd_pipe = (*mode == 'r') ? pip[1] : pip[0];

	switch (p_lastpop = fork()) {
	case 0:
		/* Child - run command */
		close(par_pipe);
		if (chd_pipe != (*mode == 'r' ? 1 : 0)) {
			dup2(chd_pipe, (*mode == 'r' ? 1 : 0));
			close(chd_pipe);
		}
		setuid(getuid());
		setgid(getgid());
		execcmd(cmd);
		exit(-1);
	case -1:
		close(chd_pipe);
		close(par_pipe);
		return ((FILE *) 0);
	default:
		close(chd_pipe);
		return (f_lastpop = fdopen(par_pipe, mode));
	}
}


/*
 * UPCLOSE -- Close the last upopened() pipe.  Unlike popen() this does not
 * take an argument.
 */

void
upclose()
{
	int pid;
	if (f_lastpop == NULL || fclose(f_lastpop))
		return;

	f_lastpop = NULL;
	while ((pid = wait((int *) 0)) != -1 && pid != p_lastpop);
}


/*
 * USYSTEM:  A modified version of the system() command that resets the uid
 * and the gid to the user before executing the subcommand.  If the command
 * doesn't appear to include any special characters, it will exec the command
 * directly instead of starting a shell to parse it.
 */

void
usystem(char *cmd)
{
	register int cpid, wpid;
	void (*old_intr) (), (*old_quit) ();

	if ((cpid = fork()) == 0) {
		dup2(2, 1);
		setuid(getuid());
		setgid(getgid());
		execcmd(cmd);
		exit(-1);
	}
	old_intr = signal(SIGINT, SIG_IGN);
	old_quit = signal(SIGQUIT, SIG_IGN);
	while ((wpid = wait((int *) 0)) != cpid && wpid != -1);
	signal(SIGINT, old_intr);
	signal(SIGQUIT, old_quit);
}

void
execcmd(char *cmd)
{
	char *cmdv[10];
	char *cp, *shell;
	int i;
	char *strpbrk();

	/* If there are no fancy characters in it, parse it ourselves */
	if (strpbrk(cmd, "<>*?|![]{}~`$&';\\\"") == NULL) {
		/* Skip leading spaces */
		cp = firstout(cmd, " \t");
		cmdv[i = 0] = cp;

		/* Break up args at the spaces */
		while (*(cp = firstin(cp, " \t")) != '\0') {
			*(cp++) = '\0';
			cp = firstout(cp, " \t");
			if (*cp != '\0')
				cmdv[++i] = cp;
		}

		/* Ignore Null command */
		if (cmdv[0] == cp)
			return;
		cmdv[i + 1] = NULL;

		execvp(cmdv[0], cmdv);
		fprintf(stderr, "%s: cannot execute %s\n", progname, cmdv[0]);
	} else {
		shell = getenv("SHELL");
		execl(shell, shell, "-c", cmd, (char *) NULL);
		fprintf(stderr, "%s: cannot execute shell %s\n",
			progname, shell);
	}
}
