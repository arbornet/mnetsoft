#include "gate.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifndef WIFEXITED
#define WIFEXITED(s)   (((s) & 0xff) == 0)
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#endif
#ifndef WIFSTOPPED
#define WIFSTOPPED(s)  (((s) & 0xff) == 0x7f && (((s) >> 8) & 0xff) != 0)
#endif
#ifndef WSTOPSIG
#define WSTOPSIG WEXITSTATUS
#endif


/* DOPIPE - Run the process with the current file as input, and replace the
 * file with its output.  It doesn't really do any piping.
 */

void dopipe(char *cmd)
{
    FILE *cfp;
    int ch;
#ifdef HAVE_SIGACTION
    struct sigaction old_intr, old_quit;
#else
    RETSIGTYPE (*old_intr)(), (*old_quit)();
#endif

    if ((cfp= pipe_thru(cmd)) == NULL) return;

    /* Copy the result in...with interupts disabled */
#ifdef HAVE_SIGACTION
    sigact(SIGINT, SIG_IGN, &old_intr);
    sigact(SIGQUIT, SIG_IGN, &old_quit);
#else
    old_intr = signal(SIGINT,SIG_IGN);
    old_quit = signal(SIGQUIT,SIG_IGN);
#endif

    emptyfile();
    while ((ch= fgetc(cfp)) != EOF)
	    putc(ch,tfp);
    fclose(cfp);

#ifdef HAVE_SIGACTION
    sigaction(SIGINT, &old_intr, NULL);
    sigaction(SIGQUIT, &old_quit, NULL);
#else
    signal(SIGINT,old_intr);
    signal(SIGQUIT,old_quit);
#endif
}


/* PIPE_THRU - This returns the descriptor of an open file containing the
 * result of piping the current buffer through the given command.  The file
 * is already rewound, so you can read it, and unlinked so that closing it
 * removes it.
 */

FILE *pipe_thru(char *cmd)
{
    char tmpname[20];
    FILE *cfp;
    long oldt;

    /* We could use a call to make_copy() here, but this method survives
     * interupts better
     */
    strcpy(tmpname, "/tmp/gateXXXXXX");
#if HAVE_MKSTEMP
    cfp= fdopen(mkstemp(tmpname),"w+");
#else
    mktemp(tmpname);
    cfp= fopen(tmpname,"w+");
#endif
    if (cfp == NULL)
    {
	printf("Cannot open file %s.\n",tmpname);
	return NULL;
    }
    unlink(tmpname);

    oldt= ftell(tfp);
    fseek(tfp,0L,0);
    if (usystem(cmd,0,1,fileno(tfp),fileno(cfp)) != 0)
	return NULL;

    fseek(cfp,0L,0);
    fseek(tfp,oldt,0);
    return cfp;
}


/* DOSYSTEM - Do a system call
 */
int dosystem(char *cmd)
{
    int rc;

    *firstin(cmd,"\n")= '\0';
    if (*cmd == '\0')
	rc= usystem(getenv("SHELL"),0,1,-1,-1);
    else
	rc= usystem(cmd,0,1,-1,-1);
    lines_above= 0;
    return (rc==127);
}


#ifdef PICO_BUG
int have_stopped;

RETSIGTYPE tmpsusp()
{
#ifdef HAVE_SIGACTION
    sigset_t set;

    sigact(SIGTSTP, SIG_DFL, NULL);

    sigemptyset(&set);
    sigaddset(&set,SIGTSTP);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
#else
#ifdef HAVE_SIGMASK
    int mask= sigblock(sigmask(SIGTSTP));
#endif

    signal(SIGTSTP,SIG_DFL);
#ifdef HAVE_SIGMASK
    sigsetmask(0);
#endif
#endif
    kill(0,SIGTSTP);

    /* STOP HERE */

#ifdef HAVE_SIGACTION
    sigact(SIGTSTP, tmpsusp, NULL);
#else
#ifdef HAVE_SIGMASK
    sigsetmask(mask);
#endif
    signal(SIGTSTP,tmpsusp);
#endif

    have_stopped= 1;
}
#endif


/* USYSTEM:  A modified version of the system() command.  If noshell is true,
 * then it executes the command directly, without starting a shell.  If sin or
 * sout, are non-negative, then they are the file descriptors to use for
 * stdin and stdout for the child process.  Returns 127 if the child process
 * was interupted or the shell was not executable.  If cook is set, reset to
 * cooked modes before doing it, and after doing it, reread the modes (they
 * may have been changed) and then restore cbreak mode, if we were in it.
 */

int usystem(char *cmd, int noshell, int cook, int sin, int sout)
{
    register pid_t cpid,wpid;
    int status;
    int was_cbreak= in_cbreak;
#ifdef HAVE_SIGACTION
    struct sigaction old_intr, old_quit, old_susp;

    sigact(SIGINT, SIG_IGN, &old_intr);
    sigact(SIGQUIT, SIG_IGN, &old_quit);
#else
    RETSIGTYPE (*old_intr)(), (*old_quit)(), (*old_susp)();

    old_intr = signal(SIGINT,SIG_IGN);
    old_quit = signal(SIGQUIT,SIG_IGN);
#endif

    if (cook) set_mode(0);

#ifdef HAVE_SIGACTION
#ifdef PICO_BUG
    have_stopped= 0;
    sigact(SIGTSTP, tmpsusp, &old_susp);
#else
    sigact(SIGTSTP, SIG_DFL, &old_susp);
#endif
#else
#ifdef PICO_BUG
    have_stopped= 0;
    old_susp = signal(SIGTSTP,tmpsusp);
#else
    old_susp = signal(SIGTSTP,SIG_DFL);
#endif
#endif

    if ((cpid = fork()) == 0)
    {
	if (sin >= 0) dup2(sin,0);
	if (sout >= 0) dup2(sout,1);
	execcmd(cmd,noshell);
	exit(127);
    }
#ifdef PICO_BUG
    while (((wpid = wait3(&status,WUNTRACED,NULL)) != -1 || errno == EINTR) &&
           (wpid != cpid || WIFSTOPPED(status)))
    {
	/* Check for god-danged "pico" which suspends itself but not it's
	 * process group - if child suspends, but I don't, suspend myself */
	if (wpid == cpid && WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP)
	{
		sleep(2);
		if (!have_stopped) kill(0,SIGTSTP);
		have_stopped= 0;
	}
    }
#else
    while (((wpid= wait(&status)) != -1 || errno == EINTR) && wpid != cpid)
	;
#endif

    if (cook)
    {
	if (wpid != -1) initmodes();
	if (was_cbreak) set_mode(1);
    }
#ifdef HAVE_SIGACTION
    sigaction(SIGINT,&old_intr,NULL);
    sigaction(SIGQUIT,&old_quit,NULL);
    sigaction(SIGTSTP,&old_susp,NULL);
#else
    signal(SIGINT,old_intr);
    signal(SIGQUIT,old_quit);
    signal(SIGTSTP,old_susp);
#endif

    return ((wpid != -1 && WIFEXITED(status)) ? WEXITSTATUS(status) : 127);
}

void execcmd(char *cmd, int noshell)
{

    /* If there are no fancy characters in it, parse it ourselves */
    if (noshell)
    {
	char *cmdv[200];
	char *cp;
	int i,j;

	/* Skip leading spaces */
	cp= firstout(cmd," \t");
	cmdv[i=0] = cp;

	/* Eliminate tailing newline - if any */
	*firstin(cmd,"\n")= '\0';

	/* Break up args at the spaces */
	while (*(cp= firstin(cp," \t")) != '\0')
	{
	    *(cp++) = '\0';
	    cp= firstout(cp," \t");
	    if (*cp != '\0') cmdv[++i] = cp;
	}

	/* Ignore Null command */
	if (cmdv[0] == cp) return;
	cmdv[i+1] = NULL;

	execvp(cmdv[0],cmdv);
	fprintf(stderr,"Cannot execute %s.\n",cmdv[0]);
    }
    else
    {
	char *shell= getenv("SHELL");
	if (shell == NULL) shell= "/bin/sh";

	execl(shell,shell,"-c",cmd,(char *)NULL);

	fprintf(stderr,"Cannot execute shell %s\n",shell);
    }
}


/* Supply this for those few stone-age systems that don't have a dup2()
 * call.
 */

#ifndef HAVE_DUP2
int dup2(int old, int new)
{
    if (old == new) return 0;
    close(new);
    if (fcntl(old,F_DUPFD,new) != new)
	    return -1;
    else
	    return new;
}
#endif /* !HAVE_DUP2 */


/* UPOPEN/UPCLOSE/UPKILL - Run command on a pipe
 *
 *    This is similar to the Unix popen() and pclose() calls, except
 *      (1)     upopen() shuts off interrupts in the child process.
 *      (2)     upopen() closes the last upopen() whenever it is called.
 *      (3)     upopen() doesn't start a shell if noshell is true.
 *      (4)     upclose() closes the last upopen(), and takes no args.
 *    upkill() just murders the child process and returns.
 */

FILE *f_lastpop= NULL;		/* current upopened stream  (NULL means none) */
pid_t p_lastpop;		/* process id of last upopened command */

FILE *upopen(char *cmd, int noshell, char *mode)
{
int fd;

    if (f_lastpop) upclose();

    if (*mode == 'r')
	p_lastpop= cmd_pipe(cmd, noshell, NULL, &fd);
    else
	p_lastpop= cmd_pipe(cmd, noshell, &fd, NULL);

    if (p_lastpop < 0)
	return NULL;
    else
	return (f_lastpop=fdopen(fd,mode));
}

void upclose()
{
    pid_t pid;

    if (f_lastpop == NULL) return;

    fclose(f_lastpop);
    f_lastpop=NULL;

    /* Wait for it to terminate */
    while ((pid=wait((int *)NULL)) != -1 && pid != p_lastpop )
	    ;
}

void upkill()
{
    if (f_lastpop != NULL)
    {
	fclose(f_lastpop);
	f_lastpop=NULL;
	kill(p_lastpop,SIGTERM);
    }
}


/* Run the given command and return the file descriptors of a pipe to write
 * to it's standard input, and one to read from it's standard output.  If
 * either of these is NULL, that pipe is not created.  It returns the child's
 * process id number, or -1 in case of failure.
 */

pid_t cmd_pipe(char *cmd, int noshell, int *to, int *from)
{
    int to_pipe[2], from_pipe[2];
    pid_t child;

    /* Make the pipes */
    if (to != NULL)
    {
	if (pipe(to_pipe)) return -1;
	*to= to_pipe[1];
    }

    if (from != NULL)
    {
	if (pipe(from_pipe)) return -1;
	*from= from_pipe[0];
    }

    switch (child= fork())
    {
    case 0:
	/* Child - run command */
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);

	if (from != NULL)
	{
	    close(from_pipe[0]);
	    if (dup2(from_pipe[1],1) == -1)
	    {
		fprintf(stderr, "Panic: can't dup \"from\" pipe\n");
		exit(1);
	    }
	}

	if (to != NULL)
	{
	    close(to_pipe[1]);
	    if (dup2(to_pipe[0],0) == -1)
	    {
		fprintf(stderr, "Panic: can't dup \"to\" pipe\n");
		exit(1);
	    }
	}

	/* setuid(getuid());
	   setgid(getgid()); */
	execcmd(cmd,noshell);
	exit(-1);
    case -1:
	/* Fork Failed */
	close(to_pipe[0]);
	close(to_pipe[1]);
	close(from_pipe[0]);
	close(from_pipe[1]);
	return -1;
    default:
	/* Happy Parent Process after fork */
	if (from != NULL) close(from_pipe[1]);
	if (to != NULL) close(to_pipe[0]);
	return child;
    }
}
