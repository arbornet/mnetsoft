#include "sys.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

extern char *progname;
int in_cbreak = 0;

/* INITMODES: Set up the cooked and cbreak modes for future use.  Note that
 * this doesn't actually change the modes.
 */

void 
initmodes()
{
	/* Get ioctl modes */
	errno = 0;
	GTTY(0, &cooked);
	if (errno != 0) {
		fprintf(stderr, "%s: Input not a tty\n", progname);
		exit(1);
	}
	cbreak = cooked;	/* Structure assignment works? */
	cbreak.c_lflag &= ~(ECHO | ICANON);
	cbreak.c_cc[VMIN] = 1;
	cbreak.c_cc[VTIME] = 0;
	cbreak.c_oflag &= ~OCRNL;
}


/*
 * SETMODE - toggles between cbreak and cooked mode.  Given a 1, it sets
 * cbreak, given a 0 it sets cooked.
 */
void 
set_mode(int into_cbreak)
{
	in_cbreak = into_cbreak;
	if (into_cbreak)
		STTY(0, &cbreak);
	else
		STTY(0, &cooked);
}


/*
 * INTR - Interupt Driver.  Fix modes and exit
 */
void 
intr()
{
	putchar('\n');
	signal(SIGINT, SIG_IGN);
	done(1);
}


/*
 * DONE - clean up tty modes before exiting.
 */
void 
done(int rc)
{
	if (in_cbreak)
		set_mode(0);
	exit(rc);
}


/*
 * SUSP - ^Z handler. Trap it, reset modes, send ourselves a suspend, then,
 * when we restart, reread the modes (they may have been changed) and
 * then restore cbreak mode, if we were in it.  Finally us the longjump to
 * resume cleanly.
 */
void 
susp()
{
	int was_cbreak = in_cbreak;
	int mask = sigblock(sigmask(SIGTSTP));
	if (in_cbreak)
		set_mode(0);

	signal(SIGTSTP, SIG_DFL);
	sigsetmask(0);
	kill(0, SIGTSTP);

	/* STOP HERE */

	sigsetmask(mask);
	signal(SIGTSTP, (void (*) ()) susp);

	initmodes();
	if (was_cbreak)
		set_mode(1);
}
