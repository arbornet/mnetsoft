/*
 * grexmore.c -- Grex version of `more'; this silly little version of this
 * program pipe's a file or standard input through tail to chop off one or
 * more lines, and then through the user's pager.  It is meant to be the
 * interpreter for shell scripts that are nothing but files to be printed
 * to the user.
 *
 *	Dan Cross <cross@cyberspace.org>
 *
 * $Id: grexmore.c 1570 2017-05-18 01:15:42Z cross $
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

char *prog;

static void usage(char *);

char DEFPAGER[] = "more -cdeiu";

void
grexmore(char *file, int start)
{
	pid_t pid;
	int fds[2];
	char *pager, *pageargs[64], **ep, **pp, opt[64];

	/* Sanity check. */
	if ((file == NULL || strcmp(file, "-") == 0) && isatty(0)) {
		warning("Can't read from a terminal.");
		return;
	}
	/* Now, actually read. */
	snprintf(opt, sizeof(opt), "%+d", start);
	pid = fork();
	if (pid < 0)
		fatal("can't fork: %r");
	/* Parent. */
	if (pid != 0) {
		wait(NULL);
		return;
	}
	/* Child. */
	pipe(fds);
	pid = fork();
	if (pid < 0)
		fatal("child can't fork: %r");
	/* Child. */
	if (pid == 0) {
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		execlp("tail", "tail", opt, file, (char *) NULL);
		fatal("can't exec tail: %r");
	}
	/* Parent; this is what grexmore is wait(2)'ing for. */
	pager = getenv("PAGER");
	if (pager == NULL)
		pager = DEFPAGER;
	pp = pageargs;
	ep = &pageargs[sizeof(pageargs) / sizeof(pageargs[0]) - 1];
	while ((*pp = strsep(&pager, " \t")) != NULL)
		if (**pp != '\0')
			if (++pp >= ep)
				break;
	*pp = NULL;
	close(fds[1]);
	dup2(fds[0], STDIN_FILENO);
	execvp(pageargs[0], pageargs);
	fatal("can't exec pager: %r");
}

int
main(int argc, char *argv[])
{
	int ch, start;

	start = 0;
	prog = progname(argv[0]);
	while ((ch = getopt(argc, argv, "n:?")) != -1) {
		switch (ch) {
		case 'n':
			start = atoi(optarg);
			break;
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (*argv == NULL)
		grexmore(NULL, start);
	else
		while (*argv)
			grexmore(*argv++, start);

	return (0);
}

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -n lineno ].\n", prog);
	exit(EXIT_FAILURE);
}
