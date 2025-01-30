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
#include <utmp.h>

#include "libgrex.h"

enum {
	SNOOZE = 10
};

typedef struct person Person;
struct person {
	char *user;
	char *line;
};

const int DEBUG = 0;

char *prog;
char *mytty;
void *people;
void *wason;

char bell = '\a';

void usage(char *);
void initialize(char *[]);
int initdb(void);
Person *getperson(char *);
int interesting(char *);
void delta(int);

int
main(int argc, char *argv[])
{
	int ch, fd;

	prog = progname(argv[0]);
	while ((ch = getopt(argc, argv, "s?")) != -1) {
		switch (ch) {
		case 's':
			bell = ' ';
			break;
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;
	initialize(argv);
	if (!DEBUG && isatty(0))
		background();
	printf("Started %s process %d on %s\n", prog, getpid(), mytty);
	fd = initdb();
	for (;;) {
		delta(fd);
		sleep(SNOOZE);
	}
	/* NOTREACHED */
	close(fd);

	return (EXIT_SUCCESS);
}

int
interesting(char *name)
{
	return (tfind(name, &people, cmpstring) != NULL);
}

static int
cmpline(const void *a, const void *b)
{
	return (strcmp(((Person *) a)->line, ((Person *) b)->line));
}

Person *
getperson(char *line)
{
	Person key, *p, **pp;

	memset(&key, 0, sizeof(key));
	key.line = line;
	pp = tfind(&key, &wason, cmpline);
	if (pp == NULL) {
		p = malloc(sizeof(*p));
		if (p == NULL)
			fatal("Can't malloc: %r");
		memset(p, 0, sizeof(*p));
		p->line = strdup(line);
		if (p->line == NULL)
			fatal("Can't strdup \"%s\": %r", line);
		pp = tsearch(p, &wason, cmpline);
	}
	return (*pp);
}

void
delta(int fd)
{
	char user[UT_NAMESIZE + 1];
	char line[UT_LINESIZE + 1];
	struct utmp ut;
	Person *p;
	static int seenme;

	while (read(fd, &ut, sizeof(ut)) == sizeof(ut)) {
		strncpy(user, ut.ut_name, UT_NAMESIZE);
		strncpy(line, ut.ut_line, UT_LINESIZE);
		user[UT_NAMESIZE] = '\0';
		line[UT_LINESIZE] = '\0';

		/*
		 * Sometimes this happens in utmp; a place holder
		 * that's never been filed in.
		 */
		if (line[0] == '\0')
			continue;
		/*
		 * If it's our line, and it's not the first time we've
		 * seen it (i.e., initial scan through utmp), exit.
		 */
		if (strcmp(line, mytty) == 0 && seenme++ != 0)
			exit(EXIT_SUCCESS);
		/*
		 * Otherwise, get the line and if there was a previous
		 * user, say s/he left.  If there is a new user, say
		 * s/he entered.
		 */
		p = getperson(line);
		if (p->user != NULL) {
			printf("out:%c   %s (%s)\r\n", bell, p->user, line);
			free(p->user);
			p->user = NULL;
		} else if (user[0] != '\0' && interesting(user)) {
			printf("in:%c    %s (%s)\r\n", bell, user, line);
			p->user = strdup(user);
			if (p->user == NULL)
				fatal("can't strdup \"%s\": %r", user);
		}
	}
}

void
initialize(char *argv[])
{
	Person e;

	memset(&e, 0, sizeof(e));
	if ((mytty = ttyname(0)) == NULL) {
		fprintf(stderr, "%s: Could not identify your terminal\n", prog);
		exit(EXIT_FAILURE);
	}
	mytty += strlen("/dev/");
	people = NULL;
	wason = NULL;
	while (*argv != NULL)
		tsearch(*argv++, &people, cmpstring);
	if (!DEBUG) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
	}
}

int
initdb(void)
{
	int wtmpfd, fd;

	wtmpfd = open(WTMP, O_RDONLY);
	if (wtmpfd < 0)
		fatal("Can't open %s: %r", WTMP);
	lseek(wtmpfd, (off_t) 0, SEEK_END);
	fd = open(UTMP, O_RDONLY);
	if (fd < 0)
		fatal("Can't open %s: %r", UTMP);
	delta(fd);
	close(fd);

	return (wtmpfd);
}

void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -s ]\n", prog);
	exit(EXIT_FAILURE);
}
