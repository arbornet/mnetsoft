/* PARTY PROGRAM -- FILTER MANAGEMENT AND SIMILAR TRICKY STUFF -- Jan Wolter */

#include "party.h"
#include "opt.h"

int out_fd = 1;		/* File descriptor of terminal or external filter */
char ofilter[BFSZ];	/* Command line of current filter */

/* START_FILTER:  Start up the external filter named by the filter option.
 * If some different filter is already running, we kill it first.
 */

void start_filter()
{
    FILE *out_fp;

    /* If a filter is running, and it is different, kill it */
    if (out_fd != 1)
    {
	if (!strcmp(ofilter,opt[OPT_FILTER].str))
	    return;
	else
	    stop_filter();
    }

    if ((out_fp = upopen(opt[OPT_FILTER].str,"w")) == NULL)
    {
	err("cannot execute filter: %s\n",ofilter);
	opt[OPT_FILTER].yes = 0;
	return;
    }
    out_fd = fileno(out_fp);
    strcpy(ofilter,opt[OPT_FILTER].str);
}

/* STOP_FILTER:  This shuts down any output filter currently running in
 * a nice manner (by closing it's input stream), and resets output direct
 * to the terminal.  This is the normal method of shutting down a filter.
 */

void stop_filter()
{
    if (out_fd == 1)
	return;
    upclose();
    out_fd= 1;
}

/* KILL_FILTER:  This shuts down any output filter currently running in
 * a nasty manner (by killing the process), and resets output direct
 * to the terminal.  This is done when party was terminated by an interrupt
 * or a hangup.
 */

void kill_filter()
{
    if (out_fd == 1)
	return;
    upkill();
    out_fd= 1;
}


/* UPOPEN/UPCLOSE/UPKILL - Run command on a pipe
 *
 *    This is similar to the Unix popen() and pclose() calls, except
 *      (1)     upopen() runs the command with the original user id.
 *      (2)     upopen() shuts off interrupts in the child process.
 *      (2)     upopen() closes the last upopen() whenever it is called.
 *      (3)     upclose() closes the last upopen(), and takes no args.
 *    upkill() just murders the child process and returns.
 */

FILE *f_lastpop = NULL;		/* current upopened stream  (NULL means none) */
int p_lastpop;			/* process id of last upopened command */

FILE *upopen(char *cmd, char *mode)
{
    int pip[2];
    register int chd_pipe,par_pipe;
    FILE *fdopen();
#ifndef HAVE_DUP2
    int t;
#endif

    if (f_lastpop) upclose();

    /* Make a pipe */
    if (pipe(pip)) return((FILE *)0);

    /* Choose ends */
    par_pipe= (*mode == 'r') ? pip[0] : pip[1];
    chd_pipe= (*mode == 'r') ? pip[1] : pip[0];

    switch (p_lastpop= fork())
    {
    case 0:
	/* Child - run command */
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	close(par_pipe);
	if (chd_pipe != (*mode == 'r'?1:0))
	{
#ifndef HAVE_DUP2
	    close(t = (*mode == 'r'?1:0));
	    if (fcntl(chd_pipe,F_DUPFD,t) != t)
	    {
		printf("Panic: can dup pipe\n");
		exit(1);
	    }
#else
	    dup2(chd_pipe,(*mode == 'r'?1:0));
#endif /*HAVE_DUP2*/
	    close(chd_pipe);
	}
	setuid(getuid());
	setgid(getgid());
	execcmd(cmd);
	exit(-1);
    case -1:
	close(chd_pipe);
	close(par_pipe);
	return (FILE *)0;
    default:
	close(chd_pipe);
	return f_lastpop=fdopen(par_pipe,mode);
    }
}

void upclose()
{
    int pid;

    if (f_lastpop == NULL || fclose(f_lastpop)) return;

    f_lastpop=NULL;
    while ((pid=wait((int *)0)) != -1 && pid != p_lastpop )
	    ;
}

void upkill()
{
    if (f_lastpop != NULL)
	kill(p_lastpop,SIGTERM);
}

/* PRINTEXEC: Print the given string, unless the first characters is a !
 * in which case we exec it.
 */

void printexec(FILE *fp, char *str)
{
    if (*str == '!')
	usystem(str+1);
    else if (*str == '/')
	help(str,1);
    else
	fprintf(fp,"%s\n",str);
}

/* USYSTEM:  A modified version of the system() command that resets the uid
 * and the gid to the user before executing the subcommand.  If the command
 * doesn't appear to include any special characters, it will exec the
 * command directly instead of starting a shell to parse it.
 */

void usystem(char *cmd)
{
    register int cpid,wpid;
    RETSIGTYPE (*old_intr)(), (*old_quit)();

    if (debug) db("usystem: %s\n",cmd);

    if ((cpid = fork()) == 0)
    {
	dup2(2,1);
	setuid(getuid());
	setgid(getgid());
	signal(SIGPIPE,oldsigpipe);
	execcmd(cmd);
	exit(-1);
    }
    old_intr = signal(SIGINT,SIG_IGN);
    old_quit = signal(SIGQUIT,SIG_IGN);
    while ((wpid = wait((int *)0)) != cpid && wpid != -1)
	;
    signal(SIGINT,old_intr);
    signal(SIGQUIT,old_quit);

    if (debug) db("usystem done\n");
}

void execcmd(char *cmd)
{
    char *cmdv[200];
    char *cp;
    int i,j;

    /* If there are no fancy characters in it, parse it ourselves */
    if (opt[OPT_FASTSHELL].yes &&
	strpbrk(cmd,"<>*?|![]{}~`$&';\\\"") == NULL)
    {
	/* Skip leading spaces */
	cp= firstout(cmd," \t");
	cmdv[i=0] = cp;

	/* Break up args at the spaces */
	while (*(cp= firstin(cp," \t")) != '\0')
	{
	    *(cp++) = '\0';
	    cp= firstout(cp," \t");
	    if (*cp != '\0')
		cmdv[++i] = cp;
	}

	/* Ignore Null command */
	if (cmdv[0] == cp) return;
	cmdv[i+1] = NULL;

	execvp(cmdv[0],cmdv);
	fprintf(stderr,"%s: cannot execute %s\n",progname,cmdv[0]);
    }
    else
    {
	execl(opt[OPT_SHELL].str,leafname(opt[OPT_SHELL].str),"-c",
		cmd,(char *)NULL);
	fprintf(stderr,"%s: cannot execute shell %s\n",
		progname,opt[OPT_SHELL].str);
    }
}
