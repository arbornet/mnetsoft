#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>

/* for setjmp/longjmp stuff */
jmp_buf env;

/* sequences that request terminal type */
#define ENQ         "\005"
#define DECID       "\033Z"
#define DA          "\033[c"
char *req[] = {DECID, DA, ENQ, NULL};
char **reqp;

/*
 * reply buffer.  Put terminal's answerback message here. (Note we rely on
 * fact that C initializes static storage to 0's to guarantee that message is
 * null-terminated.)
 */
#define REPMAX      64
char reply[REPMAX];
char *rp;			/* pointer to reply */

/*
 * old and new tty settings stored here.  (program needs to put the terminal
 * in noecho and cbreak modes)
 */
struct termios sgo, sgn;

void die(int);			/* interrupt signal handler */
void lookup(int);		/* lookup terminal type */

main()
{
	/*
	 * check that stderr is connected to the tty.  We need this because
	 * stdout may be redirected
	 */
	if (!isatty(2))
		exit(2);

	/*
	 * get current tty modes and trap interrupt signal. (will need to
	 * restore modes on premature termination)
	 */
	tcgetattr(0, &sgo);
	signal(SIGINT, die);

	/* set new modes - turn echo off and cbreak on */
	sgn = sgo;
	sgn.c_lflag &= ~(ECHO | ICANON);
	sgn.c_cc[VMIN] = 1;
	sgn.c_cc[VTIME] = 0;
	tcsetattr(0, TCSADRAIN, &sgn);

	/*
	 * output each request sequence in turn to stderr.  As soon as one
	 * returns a recognized answerback message, output the terminal type
	 * and exit.  If none do, output "dumb".
	 */
	reqp = req;
loop:
	setjmp(env);
	if (!*reqp) {
		fputs("dumb", stdout);
		die(0);
	}
	signal(SIGALRM, lookup);
	alarm(5);
	fputs(*reqp, stderr);
	fflush(stderr);
	for (rp = reply; rp < reply + REPMAX - 1; ++rp)
		*rp = getchar();

	/* should never get here */
	exit(3);
}

/* interrupt signal handler - restore tty modes and quit */
void
die(int sig)
{
	tcsetattr(0, TCSADRAIN, &sgo);
	exit(-sig);
}

/* classify terminal */
void
lookup(int sig)
{
	if (!strcmp(reply, "\033[?1c") ||
	    (!strncmp(reply, "\033[?1;", 5) && reply[strlen(reply) - 1] == 'c')) {
		fputs("vt100", stdout);
		die(0);
	}
	if (!strcmp(reply, "\033[?6c") ||
	    (!strncmp(reply, "\033[?6;", 5) && reply[strlen(reply) - 1] == 'c')) {
		fputs("vt102", stdout);
		die(0);
	}
	if (!strncmp(reply, "\033[?63", 5) && reply[strlen(reply) - 1] == 'c') {
		fputs("vt320", stdout);
		die(0);
	}
	if (!strncmp(reply, "\033[?62", 5) && reply[strlen(reply) - 1] == 'c') {
		fputs("vt220", stdout);
		die(0);
	}
	if (!strcmp(reply, "\033/Z")) {
		fputs("vt52", stdout);
		die(0);
	}
	if (!strcmp(reply, "\033/K")) {
		fputs("h19", stdout);
		die(0);
	}
	++reqp;
	longjmp(env, 0);
}
