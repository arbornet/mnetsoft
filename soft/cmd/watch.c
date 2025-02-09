/*
 * Watch logins and logouts...
 *
 * Print "in:   whatshisname" whenever someone signs on, and print "out:
 * whatshismane" whenever someone signs off.  If the "-s" option is given, no
 * bells get rung with the messages. If a list of login names is given on the
 * argument string only those people get watched.  If this program is not run
 * in background it puts itself in background.  Stops when you log off.
 *
 * This program is completely in the public domain.  It may be copied, modified,
 * traded, sold, denegrated and defoliated. This copyright notice void where
 * permitted by law.
 *
 * Revision History:
 *
 * 3/31/93  Fixed tty00 login-logout error, and added checking for
 * telnet lines (ttyp?)  - Jared Mauch
 *
 * 4/12/96  Modified things for Grex compatibility.  Automatically
 * exit when my tty logs out, which it didn't do although the
 * comments above claim it did.  - Jan Wolter
 *
 * 5/04/96  Added ajax's fix for the -s option.  - Valerie Mates
 *
 * 26-DEC-2004  Totally guted and rewrote this program.  In particular,
 * the old ``wason'' array of data structures was replaced by a BST
 * using the system tsearch(3) library (standard in Unix 98), argument
 * processing was updated to use getopt(3), error reporting is much
 * improved, everything was updated to ISO/POSIX C, a bunch of crufty
 * #ifdef's and associated goo was removed, etc.  This version is
 * much more portable, and I dare say maintainable, than the old
 * version and is about 40 lines shorter to boot.  Finally, the
 * SNOOZE time was shortened and the output now prints tty line in
 * addition to user.  - Dan Cross
 *
 * 13-FEB-2005  Modified to use libgrex.  - Dan Cross
 *
 * 03-FEB-2025  Modified to work under FreeBSD 14.  - Dan Cross
 *
 *	$Id: watch.c 1485 2014-05-21 14:18:50Z cross $
 *
 * No Copyright by Jan Dithmar Wolter.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <search.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>

#include "libgrex.h"

enum {
	SNOOZE = 2
};

#define	UT_NAMESIZE	32
#define	UT_LINESIZE	16

typedef struct line Line;
struct line {
	char line[UT_LINESIZE + 1];
};

typedef struct person Person;
struct person {
	Line line;
	char user[UT_NAMESIZE + 1];
};

const int DEBUG = 0;

char *prog;
char *mytty;
void *lines;
void *people;
void *wason;
void *seenlines;

const char *bell = "\a";
const char *interests = "/dev/null";

void usage(const char *);
void initialize(const char *, char *[]);
Line *findline(const char *);
Person *getperson(const char *);
int interesting(const char *);
void delta(void);

int
main(int argc, char *argv[])
{
	int ch;

	prog = progname(argv[0]);
	while ((ch = getopt(argc, argv, "f:s?")) != -1) {
		switch (ch) {
		case 'f':
			interests = optarg;
			break;
		case 's':
			bell = "";
			break;
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;
	initialize(interests, argv);
	if (!DEBUG && isatty(0))
		background();
	printf("Started %s process %d on %s\n", prog, getpid(), mytty);
	for (;;) {
		delta();
		sleep(SNOOZE);
	}

	return (EXIT_SUCCESS);
}

int
interesting(const char *name)
{
	return (tfind(name, &people, cmpstring) != NULL);
}

static int
cmpline(const void *a, const void *b)
{
	return (strcmp(((const Line *) a)->line, ((const Line *) b)->line));
}

static int
cmptrue(const void *a, const void *b)
{
	(void)a;
	(void)b;

	return (0);
}

Line *
findline(const char *line)
{
	Line key, *p, **pp;

	memset(&key, 0, sizeof(key));
	strlcpy(key.line, line, sizeof(key.line));
	pp = tfind(&key, &lines, cmpline);
	if (pp == NULL) {
		p = malloc(sizeof(*p));
		if (p == NULL)
			fatal("can't malloc: %r");
		memset(p, 0, sizeof(*p));
		strlcpy(p->line, line, sizeof(p->line));
		pp = tsearch(p, &wason, cmpline);
	}

	return (*pp);
}

Person *
getperson(const char *line)
{
	Person key, *p, **pp;

	memset(&key, 0, sizeof(key));
	strlcpy(key.line.line, line, sizeof(key.line));
	pp = tfind(&key, &wason, cmpline);
	if (pp == NULL) {
		p = malloc(sizeof(*p));
		if (p == NULL)
			fatal("Can't malloc: %r");
		memset(p, 0, sizeof(*p));
		strlcpy(p->line.line, line, sizeof(p->line));
		pp = tsearch(p, &wason, cmpline);
	}
	return (*pp);
}

void
prune(const posix_tnode *pp, VISIT v, int dummy)
{
	Person *p;
	Line *l;
	(void)dummy;
	(void)v;

	p = *(Person **)pp;
	if (p->user[0] == '\0')
		return;
	l = tfind(&p->line, &seenlines, cmpline);
	if (l != NULL)
		return;
	printf("out:%s   %s (%s)\r\n", bell, p->user, p->line.line);
	p->user[0] = '\0';
}

void
delta()
{
	char user[UT_NAMESIZE + 1];
	char line[UT_LINESIZE + 1];
	struct utmpx *ut;
	Person *p;
	Line *l;
	int seenme = 0;

	setutxent();
	while ((ut = getutxent()) != NULL) {
		strncpy(user, ut->ut_user, UT_NAMESIZE);
		strncpy(line, ut->ut_line, UT_LINESIZE);
		user[UT_NAMESIZE] = '\0';
		line[UT_LINESIZE] = '\0';

		/*
		 * Sometimes this happens in utmp; a place holder
		 * that's never been filed in.
		 */
		if (line[0] == '\0')
			continue;

		/*
		 * If it's our line, make a note of it.
		 */
		if (strcmp(line, mytty) == 0)
			seenme = 1;

		/*
		 * Otherwise, get the line and if there was a previous
		 * user, say s/he left.  If there is a new user, say
		 * s/he entered.
		 */
		l = findline(line);
		p = getperson(line);
		if (strcmp(p->user, user) != 0 && interesting(user)) {
			printf("in:%s    %s (%s)\r\n", bell, user, line);
			strlcpy(p->user, user, sizeof(p->user));
		}
		tsearch(l, &seenlines, cmpline);
	}
	endutxent();

	/*
	 * If we didn't see our line during the loop, we
	 * assume we logged out and exit.
	 */
	if (!seenme)
		exit(0);

	/*
	 * Look for logouts and prune the tree.
	 */
	twalk(wason, prune);
	while ((l = tdelete(l, &seenlines, cmptrue)) != NULL)
		;
}

void
initialize(const char *interests, char *argv[])
{
	Person e;
	char line[1024];
	char *u, *p;
	FILE *fp;

	memset(&e, 0, sizeof(e));
	if ((mytty = ttyname(0)) == NULL) {
		fprintf(stderr, "%s: Could not identify your terminal\n", prog);
		exit(EXIT_FAILURE);
	}
	mytty += strlen("/dev/");
	people = NULL;
	wason = NULL;
	if (!DEBUG) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
	}
	while (*argv != NULL)
		tsearch(*argv++, &people, cmpstring);
	fp = fopen(interests, "r");
	if (fp == NULL) {
		perror(interests);
		return;
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		p = strchr(line, '#');
		if (p != NULL)
			*p = '\0';
		p = line;
		while ((u = strsep(&p, ", \t\r\n\v\f")) != NULL)
			if (*u != '\0') {printf("watching '%s'\n", u);
				tsearch(u, &people, cmpstring);
}
	}
	fclose(fp);
}

void
usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [ -s ]\n", prog);
	exit(EXIT_FAILURE);
}
